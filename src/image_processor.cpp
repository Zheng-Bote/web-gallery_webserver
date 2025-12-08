#include "image_processor.hpp"
#include <QImage>
#include <QDir>
#include <QFileInfo>
#include <QDebug>

// Desired widths for the generated WebP images
const std::vector<int> ImageProcessor::TARGET_WIDTHS = {480, 680, 800, 1024, 1280};

void ImageProcessor::generateWebPVersions(const QString& sourcePath, const QString& parentDir) {
    QImage img(sourcePath);
    if (img.isNull()) {
        qWarning() << "Failed to load image for processing:" << sourcePath;
        return;
    }

    QDir dir(parentDir);
    
    // 1. Create subdirectory if it does not exist
    if (!dir.exists("webp")) {
        if (!dir.mkdir("webp")) {
            qCritical() << "Could not create 'webp' directory in" << parentDir;
            return;
        }
    }

    // Get base filename without extension (e.g. "Vacation_01")
    QString baseName = QFileInfo(sourcePath).completeBaseName();

    for (int width : TARGET_WIDTHS) {
        // Do not upscale if original is smaller
        if (img.width() <= width) continue;

        // Scale (Maintain aspect ratio, Smooth transformation)
        QImage scaled = img.scaledToWidth(width, Qt::SmoothTransformation);

        // Target path: parentDir/webp/Filename_800.webp
        QString webpFilename = QString("%1_%2.webp").arg(baseName).arg(width);
        QString targetPath = dir.filePath("webp/" + webpFilename);

        // Save (Format "WEBP", Quality 85 is a good standard)
        if (!scaled.save(targetPath, "WEBP", 85)) {
            qWarning() << "Failed to save WebP:" << targetPath;
        }
    }
}

// Deletes the webp versions
void ImageProcessor::deleteAllVersions(const QString& sourcePath) {
    QFileInfo fileInfo(sourcePath);
    QDir dir = fileInfo.dir(); // The directory where the original is located
    QString baseName = fileInfo.completeBaseName();

    if (dir.cd("webp")) { // Switch to webp subdirectory
        for (int width : TARGET_WIDTHS) {
            QString webpName = QString("%1_%2.webp").arg(baseName).arg(width);
            if (dir.exists(webpName)) {
                dir.remove(webpName);
            }
        }
    }
}