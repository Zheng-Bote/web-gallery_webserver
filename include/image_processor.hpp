#pragma once
#include <QString>
#include <vector>

/**
 * @brief Handles image processing tasks.
 * 
 * Provides functionality to generate WebP versions of images and manage them.
 */
class ImageProcessor {
public:
    /**
     * @brief Creates the webp folder and generates scaled versions of the image.
     * 
     * @param sourcePath The absolute path to the original image.
     * @param parentDir The directory where the 'webp' folder should be created.
     */
    static void generateWebPVersions(const QString& sourcePath, const QString& parentDir);

    /**
     * @brief Deletes all generated WebP versions for a file.
     * 
     * @param sourcePath The path of the source file whose versions should be deleted.
     */
    static void deleteAllVersions(const QString& sourcePath);

private:
    static const std::vector<int> TARGET_WIDTHS; ///< List of target widths for resizing.
};
