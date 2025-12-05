#pragma once
#include <string>
#include <QString>
#include <QStringList>
#include <QDateTime>

struct PhotoData {
    // Basis
    int width = 0;
    int height = 0;
    
    // Exif
    QString make, model, iso, aperture, exposure;
    double gpsLat = 0.0;
    double gpsLon = 0.0;
    double gpsAlt = 0.0;
    QDateTime takenAt;

    // IPTC / XMP / Location Logic
    QString title, description, copyright, caption;
    QString country, city, province, countryCode;
    
    // Keywords
    QStringList keywords;
};

class MetadataExtractor {
public:
    static PhotoData extract(const std::string& filepath);
};
