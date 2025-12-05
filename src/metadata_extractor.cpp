#include "metadata_extractor.hpp"
#include <exiv2/exiv2.hpp>
#include <QDebug>
#include <iostream>
#include <cmath> 

// Helper: Exiv2 String zu QString
static QString toQt(const std::string& s) {
    return QString::fromStdString(s);
}

// Helper: GPS Koordinaten berechnen
static double getGpsCoordinate(const Exiv2::ExifData& exifData, const char* key, const char* refKey) {
    try {
        auto pos = exifData.findKey(Exiv2::ExifKey(key));
        if (pos == exifData.end()) return 0.0;

        const Exiv2::Exifdatum& datum = *pos;
        if (datum.count() < 3) return 0.0;

        double degrees = datum.toRational(0).first / (double)datum.toRational(0).second;
        double minutes = datum.toRational(1).first / (double)datum.toRational(1).second;
        double seconds = datum.toRational(2).first / (double)datum.toRational(2).second;

        double decimal = degrees + (minutes / 60.0) + (seconds / 3600.0);

        // Referenz (N/S/E/W) prüfen
        auto refPos = exifData.findKey(Exiv2::ExifKey(refKey));
        if (refPos != exifData.end()) {
            std::string ref = refPos->toString();
            if (ref == "S" || ref == "W") {
                decimal *= -1.0;
            }
        }
        return decimal;

    } catch (...) {
        return 0.0;
    }
}

// Helper: Höhe berechnen
static double getGpsAltitude(const Exiv2::ExifData& exifData) {
    try {
        auto pos = exifData.findKey(Exiv2::ExifKey("Exif.GPSInfo.GPSAltitude"));
        if (pos == exifData.end()) return 0.0;

        const Exiv2::Exifdatum& datum = *pos;
        if (datum.count() < 1) return 0.0;

        double alt = datum.toRational(0).first / (double)datum.toRational(0).second;

        // Referenz prüfen
        auto refPos = exifData.findKey(Exiv2::ExifKey("Exif.GPSInfo.GPSAltitudeRef"));
        if (refPos != exifData.end()) {
            // Vorsicht bei älteren Exiv2 Versionen: toInt64() vs toLong()
            long ref = static_cast<long>(refPos->toInt64());
            if (ref == 1) {
                alt *= -1.0;
            }
        }
        return alt;
    } catch (...) {
        return 0.0;
    }
}

PhotoData MetadataExtractor::extract(const std::string& filepath) {
    PhotoData data;
    try {
        auto image = Exiv2::ImageFactory::open(filepath);
        if (!image.get()) return data;
        image->readMetadata();

        data.width = image->pixelWidth();
        data.height = image->pixelHeight();

        // --- 1. EXIF ---
        Exiv2::ExifData &exifData = image->exifData();
        if (!exifData.empty()) {
            auto getExif = [&](const char* k){ 
                auto p = exifData.findKey(Exiv2::ExifKey(k)); 
                return (p != exifData.end()) ? toQt(p->toString()) : QString(); 
            };
            data.make = getExif("Exif.Image.Make");
            data.model = getExif("Exif.Image.Model");
            data.iso = getExif("Exif.Photo.ISOSpeedRatings");
            data.aperture = getExif("Exif.Photo.FNumber");
            data.exposure = getExif("Exif.Photo.ExposureTime");
            
            std::string d = getExif("Exif.Photo.DateTimeOriginal").toStdString();
            if(!d.empty()) data.takenAt = QDateTime::fromString(toQt(d), "yyyy:MM:dd HH:mm:ss");

            data.gpsLat = getGpsCoordinate(exifData, "Exif.GPSInfo.GPSLatitude", "Exif.GPSInfo.GPSLatitudeRef");
            data.gpsLon = getGpsCoordinate(exifData, "Exif.GPSInfo.GPSLongitude", "Exif.GPSInfo.GPSLongitudeRef");
            data.gpsAlt = getGpsAltitude(exifData);
        }

        // --- 2. IPTC ---
        Exiv2::IptcData &iptcData = image->iptcData();
        if (!iptcData.empty()) {
            auto getIptc = [&](const char* k){ 
                auto p = iptcData.findKey(Exiv2::IptcKey(k)); 
                return (p != iptcData.end()) ? toQt(p->toString()) : QString(); 
            };

            data.title = getIptc("Iptc.Application2.ObjectName");
            data.caption = getIptc("Iptc.Application2.Caption");
            data.city = getIptc("Iptc.Application2.City");
            data.province = getIptc("Iptc.Application2.ProvinceState");
            data.country = getIptc("Iptc.Application2.CountryName");
            data.countryCode = getIptc("Iptc.Application2.CountryCode");
            data.copyright = getIptc("Iptc.Application2.Copyright");

            // IPTC Keywords
            auto key = Exiv2::IptcKey("Iptc.Application2.Keywords");
            auto pos = iptcData.findKey(key);
            while (pos != iptcData.end() && pos->key() == "Iptc.Application2.Keywords") {
                QString val = toQt(pos->toString());
                if (!data.keywords.contains(val)) data.keywords.append(val);
                ++pos;
            }
        }

        // --- 3. XMP (NEU HINZUGEFÜGT) ---
        Exiv2::XmpData &xmpData = image->xmpData();
        if (!xmpData.empty()) {
             // A. XMP Keywords (dc:subject)
             // XMP speichert Keywords oft als "Bag" (Liste), wir iterieren durch
             for (auto pos = xmpData.begin(); pos != xmpData.end(); ++pos) {
                 if (pos->key() == "Xmp.dc.subject") {
                     QString val = toQt(pos->toString());
                     if (!data.keywords.contains(val)) data.keywords.append(val);
                 }
             }
             
             // Helper für XMP Lookup
             auto getXmp = [&](const char* k) {
                 auto p = xmpData.findKey(Exiv2::XmpKey(k));
                 return (p != xmpData.end()) ? toQt(p->toString()) : QString();
             };

             // B. Location Fallback (Wenn in IPTC leer, nimm XMP Photoshop Namespace)
             if (data.city.isEmpty())        data.city = getXmp("Xmp.photoshop.City");
             if (data.province.isEmpty())    data.province = getXmp("Xmp.photoshop.State");
             if (data.country.isEmpty())     data.country = getXmp("Xmp.photoshop.Country");
             if (data.countryCode.isEmpty()) data.countryCode = getXmp("Xmp.iptcCore.CountryCode");
             
             // C. Info Fallback (Dublin Core)
             if (data.title.isEmpty())       data.title = getXmp("Xmp.dc.title");
             if (data.description.isEmpty()) data.description = getXmp("Xmp.dc.description");
             if (data.copyright.isEmpty())   data.copyright = getXmp("Xmp.dc.rights");
        }

    } catch (Exiv2::Error& e) {
        qWarning() << "Exiv2 Error processing" << QString::fromStdString(filepath) << ":" << e.what();
    } catch (...) {
        qWarning() << "Unknown Exiv2 Exception processing" << QString::fromStdString(filepath);
    }
    return data;
}