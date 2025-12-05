#include "controllers/upload_controller.hpp"
#include "metadata_extractor.hpp"
#include "db_manager.hpp"
#include "utils.hpp"

#include <QDir>
#include <QFile>
#include <QDateTime>
#include <QString>
#include <QDebug>

namespace routes {

void setupUploadRoutes(crow::App<crow::CORSHandler, AuthMiddleware>& app) {

    CROW_ROUTE(app, "/upload")
        .methods(crow::HTTPMethod::POST)
        .CROW_MIDDLEWARES(app, AuthMiddleware) 
    ([&app](const crow::request& req, crow::response& res){
        
        // 1. Multipart Parsen
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

        // 2. Security Checks
        if (userSubDir.contains("..") || userSubDir.contains("\\")) {
            res.code = 403; 
            res.end(R"({"error": "Security Violation: Invalid path characters."})");
            return;
        }
        while (userSubDir.startsWith("/")) userSubDir.remove(0, 1);

        // Dateiname extrahieren
        auto contentDisp = photoPart->get_header_object("Content-Disposition");
        std::string rawFilename = "unknown.jpg";
        if (contentDisp.params.find("filename") != contentDisp.params.end()) {
            rawFilename = contentDisp.params["filename"];
        }

        QString qFilename = QString::fromStdString(rawFilename);
        if (qFilename.startsWith('"') || qFilename.startsWith('\'')) qFilename.remove(0, 1);
        if (qFilename.endsWith('"') || qFilename.endsWith('\'')) qFilename.chop(1);
        
        if (!utils::isAllowedImage(qFilename.toStdString())) {
            res.code = 400;
            res.end(R"({"error": "Invalid file type"})");
            return;
        }

        // User ermitteln
        auto& ctx = app.get_context<AuthMiddleware>(req);
        QString uploader = QString::fromStdString(ctx.current_user);
        
        // TEMPORÄRER NAME (mit Prefix, um Kollisionen bei der Analyse zu vermeiden)
        QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
        QString tempFileName = QString("%1___%2___%3").arg(uploader, timestamp, qFilename);

        // FINALER NAME (Sauber, ohne Prefix)
        QString finalCleanName = qFilename; // <-- ÄNDERUNG: Wir nehmen den Originalnamen

        // 3. Temporär speichern
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
        // WORKER LOGIK
        // ---------------------------------------------------------

        // A. Metadaten aus temp Datei lesen
        PhotoData meta = MetadataExtractor::extract(tempPath.toStdString());

        // B. Zielpfad bestimmen
        QString finalRoot = "Photos"; 
        if (!userSubDir.isEmpty()) finalRoot += "/" + userSubDir;
        
        if (!QDir().mkpath(finalRoot)) {
            QFile::remove(tempPath);
            res.code = 500;
            res.end(R"({"error": "Server Error: Could not create destination directory."})");
            return;
        }

        // Zielpfad mit SAUBEREM Namen
        QString finalFullPath = finalRoot + "/" + finalCleanName; // <-- ÄNDERUNG

        // C. Verschieben (Overwrite Logik, wie in deinem ursprünglichen Worker)
        if (QFile::exists(finalFullPath)) {
            if (!QFile::remove(finalFullPath)) {
                qWarning() << "Could not overwrite existing file:" << finalFullPath;
                // Optional: Hier könnte man Logik für "image_1.jpg" einbauen
            }
        }
        
        if (!QFile::rename(tempPath, finalFullPath)) {
            qCritical() << "Failed to move file from" << tempPath << "to" << finalFullPath;
            QFile::remove(tempPath); 
            res.code = 500;
            res.end(R"({"error": "Server Error: Failed to move file to storage."})");
            return;
        }

        // D. DB Insert Payload vorbereiten
        WorkerPayload payload;
        payload.filename = finalCleanName.toStdString(); // <-- ÄNDERUNG: Sauberer Name in DB
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
        
        // URL für Frontend
        std::string urlPath = "/media/";
        if (!userSubDir.isEmpty()) urlPath += userSubDir.toStdString() + "/";
        urlPath += finalCleanName.toStdString(); // <-- ÄNDERUNG: URL nutzt sauberen Namen
        json["url"] = urlPath;
        
        res.code = 201;
        res.write(json.dump());
        res.end();
    });
}

}