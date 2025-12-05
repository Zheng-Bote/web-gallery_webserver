#include "controllers/gallery_controller.hpp"
#include "db_manager.hpp"
#include <QUuid>
#include <QSqlQuery>
#include <QVariant>

namespace routes {

void setupGalleryRoutes(crow::App<crow::CORSHandler, AuthMiddleware>& app) {

    CROW_ROUTE(app, "/api/gallery").methods(crow::HTTPMethod::GET)
    ([](const crow::request& req){
        int page = 1;
        int limit = 20;
        std::string pathFilter = "";

        if (req.url_params.get("page")) page = std::stoi(req.url_params.get("page"));
        if (req.url_params.get("path")) pathFilter = req.url_params.get("path");

        int offset = (page - 1) * limit;

        // Unique Connection Name
        QString connName = "gallery_" + QUuid::createUuid().toString();

        {
            QSqlDatabase db = DbManager::getPostgresConnection(connName);
            if (!db.isOpen()) return crow::response(500, "DB Connection Error");

            QSqlQuery q(db);
            QString sql = "SELECT p.id, p.file_name, p.file_path, l.city, l.country "
                          "FROM pictures p "
                          "LEFT JOIN meta_location l ON p.id = l.ref_picture "
                          "WHERE 1=1 ";
            
            if (!pathFilter.empty()) {
                sql += "AND p.file_path LIKE :path ";
            }

            sql += "ORDER BY p.file_datetime DESC LIMIT :lim OFFSET :off";

            q.prepare(sql);
            if (!pathFilter.empty()) {
                q.bindValue(":path", QString::fromStdString(pathFilter) + "%");
            }
            q.bindValue(":lim", limit);
            q.bindValue(":off", offset);
            
            q.exec();

            std::vector<crow::json::wvalue> list;
            while(q.next()) {
                crow::json::wvalue item;
                item["id"] = q.value("id").toInt();
                item["filename"] = q.value("file_name").toString().toStdString();
                
                std::string relPath = q.value("file_path").toString().toStdString();
                std::string fName = q.value("file_name").toString().toStdString();
                
                if (!relPath.empty() && relPath.back() != '/') relPath += "/";
                
                item["url"] = "http://beelink.local/media/" + relPath + fName; // TODO: Config Variable
                item["city"] = q.value("city").toString().toStdString();
                item["country"] = q.value("country").toString().toStdString();
                
                list.push_back(item);
            }
            crow::json::wvalue result = std::move(list);
            return crow::response(result);
        } 
        // Scope Ende -> DB Objekt zerstört -> Connection Name kann freigegeben werden
        QSqlDatabase::removeDatabase(connName);
    });
}
}
