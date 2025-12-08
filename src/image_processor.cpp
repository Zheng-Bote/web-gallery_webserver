#include "image_processor.hpp"
#include <QImage>
#include <QDir>
#include <QFileInfo>
#include <QDebug>

// Die gewünschten Breiten
const std::vector<int> ImageProcessor::TARGET_WIDTHS = {480, 680, 800, 1024, 1280};

void ImageProcessor::generateWebPVersions(const QString& sourcePath, const QString& parentDir) {
    QImage img(sourcePath);
    if (img.isNull()) {
        qWarning() << "Failed to load image for processing:" << sourcePath;
        return;
    }

    QDir dir(parentDir);
    
    // 1. Unterordner erstellen, falls nicht existent
    if (!dir.exists("webp")) {
        if (!dir.mkdir("webp")) {
            qCritical() << "Could not create 'webp' directory in" << parentDir;
            return;
        }
    }

    // Basis-Dateiname ohne Endung holen (z.B. "Urlaub_01")
    QString baseName = QFileInfo(sourcePath).completeBaseName();

    for (int width : TARGET_WIDTHS) {
        // Nicht hochskalieren, wenn das Original kleiner ist
        if (img.width() <= width) continue;

        // Skalieren (Seitenverhältnis beibehalten, Glatte Transformation)
        QImage scaled = img.scaledToWidth(width, Qt::SmoothTransformation);

        // Zielpfad: parentDir/webp/Filename_800.webp
        QString webpFilename = QString("%1_%2.webp").arg(baseName).arg(width);
        QString targetPath = dir.filePath("webp/" + webpFilename);

        // Speichern (Format "WEBP", Qualität 85 ist guter Standard)
        if (!scaled.save(targetPath, "WEBP", 85)) {
            qWarning() << "Failed to save WebP:" << targetPath;
        }
    }
}

// löschen der webp-Versionen
void ImageProcessor::deleteAllVersions(const QString& sourcePath) {
    QFileInfo fileInfo(sourcePath);
    QDir dir = fileInfo.dir(); // Der Ordner, in dem das Original liegt
    QString baseName = fileInfo.completeBaseName();

    if (dir.cd("webp")) { // In den webp Unterordner wechseln
        for (int width : TARGET_WIDTHS) {
            QString webpName = QString("%1_%2.webp").arg(baseName).arg(width);
            if (dir.exists(webpName)) {
                dir.remove(webpName);
            }
        }
    }
}