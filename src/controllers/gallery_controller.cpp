#include "controllers/gallery_controller.hpp"
#include "db_manager.hpp"
#include <QUuid>
#include <QSqlQuery>
#include <QVariant>
#include <QSqlError>
#include <QDebug>

namespace routes {

void setupGalleryRoutes(crow::App<crow::CORSHandler, AuthMiddleware>& app) {

    CROW_ROUTE(app, "/api/gallery").methods(crow::HTTPMethod::GET)
    ([](const crow::request& req){
        int page = 1;
        int limit = 100;
        std::string pathFilter = "";
        
        // Flag: Wenn true, laden wir nur Ordner (für den Baum)
        bool foldersOnly = req.url_params.get("folders_only") != nullptr;

        if (req.url_params.get("page")) page = std::stoi(req.url_params.get("page"));
        if (req.url_params.get("path")) pathFilter = req.url_params.get("path");

        // Pfad bereinigen (Slashes entfernen)
        while (!pathFilter.empty() && pathFilter.back() == '/') pathFilter.pop_back();
        while (!pathFilter.empty() && pathFilter.front() == '/') pathFilter.erase(0, 1);

        QString qPath = QString::fromStdString(pathFilter);
        QString connName = "gallery_" + QUuid::createUuid().toString();

        {
            QSqlDatabase db = DbManager::getPostgresConnection(connName);
            if (!db.isOpen()) return crow::response(500, "DB Connection Error");

            std::vector<crow::json::wvalue> responseList;

            // ---------------------------------------------------------
            // 1. UNTERORDNER FINDEN (Distinct Sub-Paths)
            // ---------------------------------------------------------
            // Wir laden Ordner meist nur auf Seite 1, um Duplikate beim Scrollen zu vermeiden
            if (page == 1) {
                QSqlQuery qFolders(db);
                QString folderSql;

                if (qPath.isEmpty()) {
                    // Root-Ebene
                    folderSql = "SELECT DISTINCT split_part(file_path, '/', 1) as folder "
                                "FROM pictures WHERE file_path <> ''";
                } else {
                    // Unterordner
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
            // 2. BILDER IM AKTUELLEN ORDNER
            // ---------------------------------------------------------
            // WICHTIG: Dieser Block darf nur übersprungen werden, wenn wir explizit nur Ordner wollen
            if (!foldersOnly) { 
                int offset = (page - 1) * limit;
                QSqlQuery qImages(db);
                
                // Wir nutzen '=' für exakten Pfad (nicht rekursiv)
                QString imgSql = R"(
                    SELECT 
                        p.id, p.file_name, p.file_path, p.file_datetime,
                        l.city, l.country,
                        e.iso, e.aperture, e.exposure_time, e.model
                    FROM pictures p
                    LEFT JOIN meta_location l ON p.id = l.ref_picture
                    LEFT JOIN meta_exif e ON p.id = e.ref_picture
                    WHERE p.file_path = :path
                    ORDER BY p.file_datetime DESC
                    LIMIT :lim OFFSET :off
                )";

                qImages.prepare(imgSql);
                qImages.bindValue(":path", qPath); // Exakter Pfad match
                qImages.bindValue(":lim", limit);
                qImages.bindValue(":off", offset);
                
                if (qImages.exec()) {
                    while(qImages.next()) {
                        crow::json::wvalue item;
                        item["id"] = qImages.value("id").toInt();
                        item["name"] = qImages.value("file_name").toString().toStdString();
                        item["filename"] = qImages.value("file_name").toString().toStdString();
                        item["type"] = "image";
                        
                        QString relDir = qImages.value("file_path").toString();
                        QString fName = qImages.value("file_name").toString();
                        
                        QString fullUrl = "/media/";
                        if (!relDir.isEmpty()) fullUrl += relDir + "/";
                        fullUrl += fName;
                        
                        item["url"] = fullUrl.toStdString();
                        
                        QDateTime dt = qImages.value("file_datetime").toDateTime();
                        if(dt.isValid()) item["date"] = dt.toString(Qt::ISODate).toStdString();
                        else item["date"] = "";

                        item["city"] = qImages.value("city").toString().toStdString();
                        item["country"] = qImages.value("country").toString().toStdString();
                        
                        QString cam = qImages.value("model").toString();
                        if (!cam.isEmpty()) item["camera"] = cam.toStdString();
                        
                        QString iso = qImages.value("iso").toString();
                        if (!iso.isEmpty()) item["iso"] = iso.toStdString();

                        responseList.push_back(item);
                    }
                } else {
                     qCritical() << "Image Query Failed:" << qImages.lastError().text();
                }
            }

            crow::json::wvalue result = std::move(responseList);
            return crow::response(result);
        } 
        QSqlDatabase::removeDatabase(connName);
    });
}
}