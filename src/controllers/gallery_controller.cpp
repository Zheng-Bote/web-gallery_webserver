/**
 * @file gallery_controller.cpp
 * @brief Implementation of Gallery Routes.
 */
#include "controllers/gallery_controller.hpp"
#include "db_manager.hpp"
#include <QSqlQuery>
#include <QVariant>
#include <QSqlError> // IMPORTANT
#include <QDebug>

namespace routes {

void setupGalleryRoutes(crow::App<crow::CORSHandler, AuthMiddleware>& app) {

    CROW_ROUTE(app, "/api/gallery").methods(crow::HTTPMethod::GET)
    ([](const crow::request& req){
        int page = 1;
        int limit = 100;
        std::string pathFilter = "";
        
        // Flag: Only load folder structure (for Navigation Tree)
        bool foldersOnly = req.url_params.get("folders_only") != nullptr;

        if (req.url_params.get("page")) page = std::stoi(req.url_params.get("page"));
        if (req.url_params.get("path")) pathFilter = req.url_params.get("path");

        // Clean up path
        while (!pathFilter.empty() && pathFilter.back() == '/') pathFilter.pop_back();
        while (!pathFilter.empty() && pathFilter.front() == '/') pathFilter.erase(0, 1);

        QString qPath = QString::fromStdString(pathFilter);

        // NEW: Get connection from pool
        QSqlDatabase db = DbManager::getPostgresConnection();
        
        if (!db.isOpen()) return crow::response(500, "DB Connection Error");

        std::vector<crow::json::wvalue> responseList;

        // ---------------------------------------------------------
        // 1. FIND SUBFOLDERS
        // ---------------------------------------------------------
        // We only load folders on page 1, to avoid duplicates when scrolling
        if (page == 1) {
            QSqlQuery qFolders(db);
            QString folderSql;

            if (qPath.isEmpty()) {
                // Root: First part of the path
                folderSql = "SELECT DISTINCT split_part(file_path, '/', 1) as folder "
                            "FROM pictures WHERE file_path <> ''";
            } else {
                // Subfolder: Part after current path
                folderSql = "SELECT DISTINCT split_part(substring(file_path, length(:base) + 2), '/', 1) as folder "
                            "FROM pictures WHERE file_path LIKE :base || '/%'";
            }
            
            qFolders.prepare(folderSql);
            if (!qPath.isEmpty()) {
                qFolders.bindValue(":base", qPath);
            }
            
            if (qFolders.exec()) {
                while(qFolders.next()) {
                    QString folderName = qFolders.value(0).toString();
                    if (folderName.isEmpty()) continue;

                    crow::json::wvalue folderItem;
                    folderItem["name"] = folderName.toStdString();
                    folderItem["type"] = "folder";
                    
                    QString fullFolderPath = qPath.isEmpty() ? folderName : qPath + "/" + folderName;
                    folderItem["path"] = fullFolderPath.toStdString();
                    
                    responseList.push_back(folderItem);
                }
            } else {
                qWarning() << "Folder Query Failed:" << qFolders.lastError().text();
            }
        }

        // ---------------------------------------------------------
        // 2. PICTURES IN CURRENT FOLDER
        // ---------------------------------------------------------
        // Only execute if we are not in pure Tree-Mode
        if (!foldersOnly) { 
            int offset = (page - 1) * limit;
            QSqlQuery qImages(db);
            
            // UPDATE: We join meta_iptc and fetch keywords via subselect (Postgres string_agg)
                QString imgSql = R"(
                    SELECT 
                        p.id, p.file_name, p.file_path, p.file_datetime,
                        l.city, l.country,
                        e.iso, e.aperture, e.exposure_time, e.model,
                        i.object_name as title, i.caption as description, i.copyright,
                        (
                            SELECT string_agg(k.tag, ',') 
                            FROM keywords k 
                            JOIN picture_keywords pk ON k.id = pk.keyword_id 
                            WHERE pk.picture_id = p.id
                        ) as keyword_string
                    FROM pictures p
                    LEFT JOIN meta_location l ON p.id = l.ref_picture
                    LEFT JOIN meta_exif e ON p.id = e.ref_picture
                    LEFT JOIN meta_iptc i ON p.id = i.ref_picture
                    WHERE p.file_path = :path
                    ORDER BY p.file_datetime DESC
                    LIMIT :lim OFFSET :off
                )";

                qImages.prepare(imgSql);
                qImages.bindValue(":path", qPath);
                qImages.bindValue(":lim", limit);
                qImages.bindValue(":off", offset);
                
                if (qImages.exec()) {
                    while(qImages.next()) {
                        crow::json::wvalue item;
                        item["id"] = qImages.value("id").toInt();
                        item["filename"] = qImages.value("file_name").toString().toStdString();
                        // We use 'name' in frontend as title if present, otherwise filename
                        QString dbTitle = qImages.value("title").toString();
                        item["name"] = dbTitle.isEmpty() ? qImages.value("file_name").toString().toStdString() : dbTitle.toStdString();
                        
                        item["type"] = "image";
                        
                        // Paths
                        QString relDir = qImages.value("file_path").toString();
                        QString fName = qImages.value("file_name").toString();
                        QString fullUrl = "/media/";
                        if (!relDir.isEmpty()) fullUrl += relDir + "/";
                        fullUrl += fName;
                        item["url"] = fullUrl.toStdString();
                        
                        // Date
                        QDateTime dt = qImages.value("file_datetime").toDateTime();
                        item["date"] = dt.isValid() ? dt.toString(Qt::ISODate).toStdString() : "";

                        // Metadata
                        item["city"] = qImages.value("city").toString().toStdString();
                        item["country"] = qImages.value("country").toString().toStdString();
                        
                        QString cam = qImages.value("model").toString();
                        if (!cam.isEmpty()) item["camera"] = cam.toStdString();
                        
                        // --- NEW: Send IPTC Data ---
                        item["title"] = dbTitle.toStdString();
                        item["description"] = qImages.value("description").toString().toStdString();
                        item["copyright"] = qImages.value("copyright").toString().toStdString();
                        
                        // Keywords come as "Tag1,Tag2,Tag3" string from DB
                        // We send it as string, the frontend splits it
                        item["keywords_string"] = qImages.value("keyword_string").toString().toStdString();

                        responseList.push_back(item);
                    }
            } else {
                 qCritical() << "Image Query Failed:" << qImages.lastError().text();
            }
        }

        crow::json::wvalue result = std::move(responseList);
        return crow::response(result);
        
        // IMPORTANT: Connection is NOT closed, remains in Pool.
    });


/**
 * @brief DELETE Photo
 * 
 */
    CROW_ROUTE(app, "/api/gallery/<int>")
        .methods(crow::HTTPMethod::DELETE)
        .CROW_MIDDLEWARES(app, AuthMiddleware)
    ([](const crow::request&, crow::response& res, int id){
        
        if (DbManager::deletePhoto(id)) {
            res.code = 200;
            res.end(R"({"status": "deleted"})");
        } else {
            res.code = 404; // Or 500, but mostly ID is not found
            res.end(R"({"error": "Could not delete photo"})");
        }
    });

    /**
     * @brief UPDATE Photo Metadata
     * 
     */
    CROW_ROUTE(app, "/api/gallery/<int>")
        .methods(crow::HTTPMethod::PUT)
        .CROW_MIDDLEWARES(app, AuthMiddleware)
    ([](const crow::request& req, crow::response& res, int id){
        
        auto json = crow::json::load(req.body);
        if (!json) {
            res.code = 400;
            res.end(R"({"error": "Invalid JSON"})");
            return;
        }

        PhotoUpdateData data;
        
        // Title & Description
        if (json.has("title")) data.title = json["title"].s();
        if (json.has("description")) data.description = json["description"].s();
        
        // Keywords (Array)
        if (json.has("keywords")) {
            for (const auto& k : json["keywords"]) {
                data.keywords.push_back(k.s());
            }
        }

        if (DbManager::updatePhotoMetadata(id, data)) {
            res.code = 200;
            res.end(R"({"status": "updated"})");
        } else {
            res.code = 500;
            res.end(R"({"error": "Update failed"})");
        }
    });
}
}