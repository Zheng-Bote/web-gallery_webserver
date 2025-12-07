#pragma once
#include <string>
#include <QString>
#include <QStringList>
#include <QDateTime>

/**
 * @brief Structure containing all extracted photo metadata.
 */
struct PhotoData {
    // Base
    int width = 0; ///< Image width in pixels.
    int height = 0; ///< Image height in pixels.
    
    // Exif
    QString make; ///< Camera maker.
    QString model; ///< Camera model.
    QString iso; ///< ISO speed.
    QString aperture; ///< F-Number.
    QString exposure; ///< Exposure time.
    double gpsLat = 0.0; ///< GPS Latitude.
    double gpsLon = 0.0; ///< GPS Longitude.
    double gpsAlt = 0.0; ///< GPS Altitude.
    QDateTime takenAt; ///< Datetime original.

    // IPTC / XMP / Location Logic
    QString title; ///< Object Name / Title.
    QString description; ///< description.
    QString copyright; ///< Copyright notice.
    QString caption; ///< Caption / Abstract.
    QString country; ///< Country name.
    QString city; ///< City name.
    QString province; ///< Province or State.
    QString countryCode; ///< ISO Country Code.
    
    // Keywords
    QStringList keywords; ///< List of keywords/tags.
};

/**
 * @brief Class for extracting metadata from images.
 * 
 * Uses Exiv2 to read Exif, IPTC, and XMP data.
 */
class MetadataExtractor {
public:
    /**
     * @brief Extracts metadata from a given file.
     * 
     * @param filepath The absolute path to the image file.
     * @return PhotoData structure with extracted values.
     */
    static PhotoData extract(const std::string& filepath);
};
