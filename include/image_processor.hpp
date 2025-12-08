#pragma once
#include <QString>
#include <vector>

class ImageProcessor {
public:
    // Erstellt den webp-Ordner und die skalierten Versionen
    static void generateWebPVersions(const QString& sourcePath, const QString& parentDir);

    // Löscht alle generierten WebP-Versionen (Vorbereitung für Delete-Feature)
    static void deleteAllVersions(const QString& sourcePath);

private:
    static const std::vector<int> TARGET_WIDTHS;
};
