#include "controllers/web_controller.hpp"
#include "rz_config.hpp" 
#include <filesystem>

namespace fs = std::filesystem;

namespace routes {

void setupWebRoutes(crow::App<crow::CORSHandler, AuthMiddleware>& app) {

    // --- SYSTEM INFO ROUTE (JSON) ---
    CROW_ROUTE(app, "/system/json")
    ([]{
        crow::json::wvalue x({{"PROJECT_NAME", PROJECT_NAME}, 
                            {"PROJECT_VERSION", PROJECT_VERSION}, 
                            {"PROJECT_DESCRIPTION", PROJECT_DESCRIPTION},
                            {"PROJECT_HOMEPAGE_URL", PROJECT_HOMEPAGE_URL},
                            {"PROG_CREATED", PROG_CREATED},
                            {"PROG_AUTHOR", PROG_AUTHOR},
                            {"PROG_ORGANIZATION_NAME", PROG_ORGANIZATION_NAME},
                            {"PROG_ORGANIZATION_DOMAIN", PROG_ORGANIZATION_DOMAIN},
                            {"PROG_LONGNAME", PROG_LONGNAME},
                            {"PROJECT_EXECUTABLE", PROJECT_EXECUTABLE},
                            {"CMAKE_CXX_STANDARD", CMAKE_CXX_STANDARD},
                            {"CMAKE_CXX_COMPILER", CMAKE_CXX_COMPILER},
                            {"CMAKE_QT_VERSION", CMAKE_QT_VERSION},
                            
                        });
        
        return x; 
    });

    // --- ROOT ROUTE (Startseite) ---
    CROW_ROUTE(app, "/")
    ([](const crow::request&, crow::response& res){
        // Liefert static/index.html aus
        res.set_static_file_info("static/index.html");
        res.end(); 
    });

    // --- STATIC FILES (CSS, JS, TXT) ---
    // Beispiel: http://localhost:8080/static/style.css
    CROW_ROUTE(app, "/static/<string>")
    ([](const crow::request&, crow::response& res, std::string filename){
        // Schutz vor Directory Traversal (einfach)
        if (filename.find("..") != std::string::npos) {
            res.code = 403;
            res.end();
            return;
        }
        res.set_static_file_info("static/" + filename);
        res.end(); 
    });

    // --- TEMPLATE TEST (Mustache) ---
    CROW_ROUTE(app, "/template")
    ([](){
        auto page = crow::mustache::load("template.html");
        crow::mustache::context ctx;
        ctx["user_name"] = "Gast";
        ctx["message"] = "Willkommen auf dem Refactored Server!";
        ctx["time_is_morning"] = false; 

        return page.render(ctx); 
    });
}

}