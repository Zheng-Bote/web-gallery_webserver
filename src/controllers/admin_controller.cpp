#include "controllers/admin_controller.hpp"
#include "db_manager.hpp"

namespace routes {

void setupAdminRoutes(crow::App<crow::CORSHandler, AuthMiddleware>& app) {

    // TRICK: Wir speichern die Adresse der App in einem Pointer.
    // Dieser Pointer ist sicher, da die App in main() lebt und nicht gelöscht wird.
    auto* appPtr = &app;

    // Helper Lambda für Security Check
    auto ensureAdmin = [](const crow::request& req, crow::response& res) -> bool {
        // AuthMiddleware hat context gesetzt
        // Wir kommen nur hierher, wenn Token gültig ist.
        // Optional: Prüfen ob User == 'admin' ist.
        // auto& ctx = app.get_context<AuthMiddleware>(req); 
        // if (ctx.current_user != "admin") ...
        return true; 
    };

    // --- 1. LISTE ALLER USER ---
    CROW_ROUTE(app, "/api/admin/users")
        .methods(crow::HTTPMethod::GET)
        .CROW_MIDDLEWARES(app, AuthMiddleware)
    ([appPtr](const crow::request& req, crow::response& res){ // <--- appPtr hier capturen!
        
        // Sicherheitscheck: Ist der User Admin?
        auto& ctx = appPtr->get_context<AuthMiddleware>(req);
        if (ctx.current_user != "admin") { 
            res.code = 403; 
            res.end("Forbidden: Admin access only"); 
            return; 
        }

        auto users = DbManager::getAllUsers();
        
        std::vector<crow::json::wvalue> jsonList;
        for (const auto& u : users) {
            crow::json::wvalue j;
            j["id"] = u.id;
            j["username"] = u.username;
            j["created_at"] = u.createdAt;
            j["is_active"] = u.isActive;
            jsonList.push_back(j);
        }
        
        crow::json::wvalue result = std::move(jsonList);
        res.end(result.dump());
    });
    // 2. USER ANLEGEN
    CROW_ROUTE(app, "/api/admin/users")
        .methods(crow::HTTPMethod::POST)
        .CROW_MIDDLEWARES(app, AuthMiddleware)
    ([](const crow::request& req, crow::response& res){
        auto json = crow::json::load(req.body);
        if (!json || !json.has("username") || !json.has("password")) {
            res.code = 400; res.end("Missing data"); return;
        }

        if (DbManager::createUser(json["username"].s(), json["password"].s())) {
            res.code = 201; 
            res.end("User created");
        } else {
            res.code = 409; 
            res.end("Failed to create user (allready exists?)");
        }
    });

    // 3. USER LÖSCHEN
    CROW_ROUTE(app, "/api/admin/users/<int>")
        .methods(crow::HTTPMethod::DELETE)
        .CROW_MIDDLEWARES(app, AuthMiddleware)
    ([](const crow::request&, crow::response& res, int id){
        // Optional: Verhindern dass man sich selbst löscht
        if (id == 1) { // Annahme: ID 1 ist Super-Admin
             res.code = 403; 
             res.end("Cannot delete root admin"); return;
        }

        if (DbManager::deleteUser(id)) {
            res.code = 200; res.end("User deleted");
        } else {
            res.code = 404; res.end("User not found");
        }
    });

    // --- 4. USER STATUS ÄNDERN (Aktivieren/Deaktivieren) ---
    CROW_ROUTE(app, "/api/admin/users/<int>/status")
        .methods(crow::HTTPMethod::PUT)
        .CROW_MIDDLEWARES(app, AuthMiddleware)
    ([appPtr](const crow::request& req, crow::response& res, int id){
        
        // Security Check (Nur Admins dürfen das)
        auto& ctx = appPtr->get_context<AuthMiddleware>(req);
        if (ctx.current_user != "admin") { 
            res.code = 403; res.end("Forbidden"); return; 
        }

        // Besonderer Schutz für ID 1 (kann nicht deaktiviert werden)
        if (id == 1) {
            res.code = 403; 
            res.end(R"({"error": "Cannot deactivate root admin"})");
            return;
        }

        auto json = crow::json::load(req.body);
        if (!json || !json.has("active")) {
            res.code = 400; res.end("Missing 'active' boolean"); return;
        }

        bool active = json["active"].b();

        if (DbManager::updateUserStatus(id, active)) {
            res.code = 200; 
            res.end(R"({"status": "updated"})");
        } else {
            res.code = 500; 
            res.end("Update failed");
        }
    });
}

}
