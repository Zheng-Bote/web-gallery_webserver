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
        std::string currentPath = ""; 
        if (req.url_params.get("path")) {
            currentPath = req.url_params.get("path");
        }

        // Bereinige führende/hintere Slashes für DB Konsistenz
        while (!currentPath.empty() && currentPath.back() == '/') currentPath.pop_back();
        while (!currentPath.empty() && currentPath.front() == '/') currentPath.erase(0, 1);

        QString qPath = QString::fromStdString(currentPath);
        QString connName = "gallery_" + QUuid::createUuid().toString();

        {
            QSqlDatabase db = DbManager::getPostgresConnection(connName);
            if (!db.isOpen()) return crow::response(500, "DB Connection Error");

            std::vector<crow::json::wvalue> responseList;

            // ---------------------------------------------------------
            // 1. UNTERORDNER FINDEN (Distinct Sub-Paths)
            // ---------------------------------------------------------
            QSqlQuery qFolders(db);
            QString folderSql;

            // Logik: Wir schauen in die Tabelle 'pictures' und extrahieren den nächsten Ordnerteil.
            // PostgreSQL Funktion: split_part
            
            if (qPath.isEmpty()) {
                // Root: Wir nehmen den ersten Teil des Pfades (z.B. "2025" aus "2025/Sommer")
                folderSql = "SELECT DISTINCT split_part(file_path, '/', 1) as folder "
                            "FROM pictures WHERE file_path <> ''";
            } else {
                // Subfolder: Wir nehmen den Teil NACH dem aktuellen Pfad
                // Beispiel: Pfad="2025". Datei="2025/Sommer/Bild.jpg".
                // Wir wollen "Sommer".
                // Wir filtern auf Pfade, die mit "2025/" beginnen, aber länger sind.
                folderSql = "SELECT DISTINCT split_part(substring(file_path, length(:base) + 2), '/', 1) as folder "
                            "FROM pictures "
                            "WHERE file_path LIKE :base || '/%'";
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
                    
                    // Der Pfad, wenn man draufklickt
                    QString fullFolderPath = qPath.isEmpty() ? folderName : qPath + "/" + folderName;
                    folderItem["path"] = fullFolderPath.toStdString();
                    
                    responseList.push_back(folderItem);
                }
            } else {
                qWarning() << "Folder Query failed:" << qFolders.lastError().text();
            }

            // ---------------------------------------------------------
            // 2. BILDER IM AKTUELLEN ORDNER
            // ---------------------------------------------------------
            QSqlQuery qImages(db);
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
            )";
            // WICHTIG: Hier nutzen wir '=' statt 'LIKE', damit wir nicht rekursiv suchen.
            // Nur Bilder, die DIREKT in diesem Ordner liegen.

            qImages.prepare(imgSql);
            qImages.bindValue(":path", qPath);
            
            if (qImages.exec()) {
                while(qImages.next()) {
                    crow::json::wvalue item;
                    item["id"] = qImages.value("id").toInt();
                    item["name"] = qImages.value("file_name").toString().toStdString(); // Einheitlich 'name'
                    item["filename"] = qImages.value("file_name").toString().toStdString(); // Legacy support
                    item["type"] = "image";
                    
                    QString relDir = qImages.value("file_path").toString();
                    QString fName = qImages.value("file_name").toString();
                    QString fullUrl = "/media/";
                    if (!relDir.isEmpty()) fullUrl += relDir + "/";
                    fullUrl += fName;
                    
                    item["url"] = fullUrl.toStdString();
                    
                    QDateTime dt = qImages.value("file_datetime").toDateTime();
                    if(dt.isValid()) item["date"] = dt.toString(Qt::ISODate).toStdString();

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

            crow::json::wvalue result = std::move(responseList);
            // Wir geben jetzt direkt eine Liste zurück, kein {data: [], page: 1} Wrapper mehr,
            // um es einfacher zu halten für die Ordnerstruktur.
            return crow::response(result);
        } 
        QSqlDatabase::removeDatabase(connName);
    });
}
}
