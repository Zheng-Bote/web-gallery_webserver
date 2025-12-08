/**
 * @file upload_controller.cpp
 * @brief Implementation of Upload Routes.
 */
#include "controllers/upload_controller.hpp"
#include "metadata_extractor.hpp"
#include "image_processor.hpp"
#include "db_manager.hpp"
#include "utils.hpp"

#include <QDir>
#include <QFile>
#include <QDateTime>
#include <QString>
#include <QDebug>
#include <QThreadPool>
#include <QRegularExpression>

namespace routes {

void setupUploadRoutes(crow::App<crow::CORSHandler, AuthMiddleware>& app) {

    CROW_ROUTE(app, "/upload")
        .methods(crow::HTTPMethod::POST)
        .CROW_MIDDLEWARES(app, AuthMiddleware) 
    ([&app](const crow::request& req, crow::response& res){
        
        // Set Payload Limit
        // Default is often 10MB. We set it to 50 MB.
        // Calculation: 50 * 1024 * 1024 Bytes
        if (req.body.size() > 52428800) {
             res.code = 413; // Payload Too Large
             res.end(R"({"error": "File too large. Max 50MB."})");
             return;
        }
        
        // 1. Parse Multipart
        crow::multipart::message msg(req);
        const crow::multipart::part* photoPart = nullptr;
        QString userSubDir = "";
        
        for (const auto& part : msg.parts) {
            auto contentDisp = part.get_header_object("Content-Disposition");
            auto it = contentDisp.params.find("name");
            
            if (it != contentDisp.params.end()) {
                if (it->second == "photo") {
                    photoPart = &part;
                } else if (it->second == "path") {
                    userSubDir = QString::fromStdString(part.body).trimmed();
                }
            }
        }

        if (!photoPart) {
            res.code = 400;
            res.end(R"({"error": "Part 'photo' missing"})");
            return;
        }

        // ---------------------------------------------------------
        // CLEAN PATH (FIX FOR DOUBLE SLASHES)
        // ---------------------------------------------------------
        
        // 1. Normalize Backslashes (Windows) to Slashes
        userSubDir.replace("\\", "/");

        // 2. Remove double/multiple slashes (// -> /)
        // Regex finds occurrences of 2 or more slashes
        static const QRegularExpression multiSlash(R"(/+)");
        userSubDir.replace(multiSlash, "/");

        // 3. Security Checks (Directory Traversal)
        if (userSubDir.contains("../") || userSubDir.contains("/..") || userSubDir == ".." || userSubDir.contains("\\")) {
            res.code = 403; 
            res.end(R"({"error": "Security Violation: Invalid path components."})");
            return;
        }

        // 4. Remove leading and trailing slashes
        while (userSubDir.startsWith("/")) userSubDir.remove(0, 1);
        while (userSubDir.endsWith("/")) userSubDir.chop(1);

        // ---------------------------------------------------------


        // Extract filename
        auto contentDisp = photoPart->get_header_object("Content-Disposition");
        std::string rawFilename = "unknown.jpg";
        if (contentDisp.params.find("filename") != contentDisp.params.end()) {
            rawFilename = contentDisp.params["filename"];
        }

        QString qFilename = QString::fromStdString(rawFilename);
        
        // 1. Remove Browser Quotes
        if (qFilename.startsWith('"') || qFilename.startsWith('\'')) qFilename.remove(0, 1);
        if (qFilename.endsWith('"') || qFilename.endsWith('\'')) qFilename.chop(1);
        
        // 2. Spaces to Underscores (good for URLs)
        qFilename.replace(" ", "_");

        // 3. Bad characters to Hyphen (Windows-Incompatible & Path-Separators)
        // We use a regex for all forbidden characters at once
        // R"(...)" is a raw string, to avoid double escaping backslashes
        static const QRegularExpression illegalChars(R"([:/\\*?"'<>|])");
        qFilename.replace(illegalChars, "-");

        // 4. Prevent double dots (Security against .. paths)
        while (qFilename.contains("..")) {
            qFilename.replace("..", ".");
        }

        // 5. Remove Control Characters (e.g. Newlines in filename)
        static const QRegularExpression controlChars(R"([\x00-\x1f\x7f])");
        qFilename.remove(controlChars);

        if (!utils::isAllowedImage(qFilename.toStdString())) {
            res.code = 400;
            res.end(R"({"error": "Invalid file type"})");
            return;
        }

        // Determine User
        auto& ctx = app.get_context<AuthMiddleware>(req);
        QString uploader = QString::fromStdString(ctx.current_user);
        
        // TEMPORARY NAME (with prefix, to avoid collisions during analysis)
        QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
        QString tempFileName = QString("%1___%2___%3").arg(uploader, timestamp, qFilename);

        // FINAL NAME (Clean, without prefix)
        QString finalCleanName = qFilename; // <-- CHANGE: We use the original name

        // 3. Save temporarily
        QString tempDir = "uploads/temp";
        QDir().mkpath(tempDir);
        QString tempPath = tempDir + "/" + tempFileName;

        QFile file(tempPath);
        if (!file.open(QIODevice::WriteOnly)) {
            res.code = 500;
            res.end(R"({"error": "Server Error: Could not save temp file."})");
            return;
        }
        file.write(photoPart->body.data(), photoPart->body.size());
        file.close();

        // ---------------------------------------------------------
        // WORKER LOGIC
        // ---------------------------------------------------------

        // A. Read metadata from temp file
        PhotoData meta = MetadataExtractor::extract(tempPath.toStdString());

        // B. Determine destination path
        QString finalRoot = "Photos"; 
        if (!userSubDir.isEmpty()) finalRoot += "/" + userSubDir;
        
        if (!QDir().mkpath(finalRoot)) {
            QFile::remove(tempPath);
            res.code = 500;
            res.end(R"({"error": "Server Error: Could not create destination directory."})");
            return;
        }

        // Destination path with CLEAN name
        QString finalFullPath = finalRoot + "/" + finalCleanName; // <-- CHANGE

        // C. Move (Overwrite Logic...)
        if (QFile::exists(finalFullPath)) {
            if (!QFile::remove(finalFullPath)) {
                qWarning() << "Could not overwrite existing file:" << finalFullPath;
                // Optional: Here logic for "image_1.jpg" could be added
            }
        }
        
        if (!QFile::rename(tempPath, finalFullPath)) {
            qCritical() << "Failed to move file from" << tempPath << "to" << finalFullPath;
            QFile::remove(tempPath); 
            res.code = 500;
            res.end(R"({"error": "Server Error: Failed to move file to storage."})");
            return;
        }

        // --- Start WebP Generation ---
        // We pass the full path to the new image and the folder
        // finalFullPath: "Photos/2025/Bild.jpg"
        // finalRoot:     "Photos/2025"
        
        // We use QThreadPool for Fire&Forget. 
        // IMPORTANT: We must copy QStrings into the lambda (capture by value),
        // as references become invalid once this request block ends.
        QThreadPool::globalInstance()->start([finalFullPath, finalRoot](){
            ImageProcessor::generateWebPVersions(finalFullPath, finalRoot);
            qDebug() << "Background processing finished for:" << finalFullPath;
        });

        // D. Prepare DB Insert Payload
        WorkerPayload payload;
        payload.filename = finalCleanName.toStdString(); // <-- CHANGE: Clean name in DB
        payload.relPath  = userSubDir.toStdString(); 
        payload.fullPath = finalFullPath.toStdString();
        payload.user     = uploader.toStdString();
        payload.fileSize = photoPart->body.size();

        if (meta.takenAt.isValid()) {
            payload.fileDate = meta.takenAt;
        } else {
            payload.fileDate = QDateTime::currentDateTime();
        }
        payload.meta = meta;

        // E. DB Insert
        bool dbSuccess = DbManager::insertPhoto(payload);

        // ---------------------------------------------------------
        // RESPONSE
        // ---------------------------------------------------------
        
        crow::json::wvalue json;
        if (dbSuccess) {
            json["status"] = "success";
            json["message"] = "File uploaded.";
        } else {
            json["status"] = "partial_success";
            json["message"] = "File saved, DB error.";
        }

        json["path"] = finalFullPath.toStdString();
        
        // URL for Frontend
        std::string urlPath = "/media/";
        if (!userSubDir.isEmpty()) urlPath += userSubDir.toStdString() + "/";
        urlPath += finalCleanName.toStdString(); // <-- CHANGE: URL uses clean name
        json["url"] = urlPath;
        
        res.code = 201;
        res.write(json.dump());
        res.end();
    });
}

}