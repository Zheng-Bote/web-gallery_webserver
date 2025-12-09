#include "controllers/auth_controller.hpp"
#include "db_manager.hpp"
#include "utils.hpp"
#include <jwt-cpp/jwt.h>

namespace routes {

void setupAuthRoutes(crow::App<crow::CORSHandler, AuthMiddleware>& app) {
    
    // WICHTIG: Diese Zeile muss oben stehen, damit die Lambdas 'appPtr' kennen
    auto* appPtr = &app;

    // --- LOGIN ---
    CROW_ROUTE(app, "/login").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req){
        auto json = crow::json::load(req.body);
        if (!json) return crow::response(400, "Invalid JSON");
        if (!json.has("username") || !json.has("password")) {
            return crow::response(400, "Missing username or password");
        }

        std::string user = json["username"].s();
        std::string pass = json["password"].s();

        if (DbManager::verifyUser(user, pass)) {
            // 1. Access Token (15 Min)
            auto accessToken = jwt::create()
                .set_issuer("crow_qt_server")
                .set_type("JWS")
                .set_payload_claim("username", jwt::claim(user))
                .set_issued_at(std::chrono::system_clock::now())
                .set_expires_at(std::chrono::system_clock::now() + std::chrono::minutes{15})
                .sign(jwt::algorithm::hs256{utils::getJwtSecret()});

            // 2. Refresh Token (Long term)
            std::string refreshToken = utils::generateRandomString(64);
            DbManager::storeRefreshToken(user, refreshToken);

            crow::json::wvalue resp;
            resp["token"] = accessToken;
            resp["refreshToken"] = refreshToken;
            return crow::response(resp);
        }
        return crow::response(401, "Invalid credentials");
    });

    // --- LOGOUT ---
    CROW_ROUTE(app, "/logout").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req){
        auto json = crow::json::load(req.body);
        if (json && json.has("refreshToken")) {
            std::string rToken = json["refreshToken"].s();
            DbManager::revokeRefreshToken(rToken);
        }
        return crow::response(200, "Logged out successfully");
    });

    // --- REFRESH ---
    CROW_ROUTE(app, "/refresh").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req){
        auto json = crow::json::load(req.body);
        if (!json || !json.has("refreshToken")) return crow::response(400);

        std::string rToken = json["refreshToken"].s();
        std::string user = DbManager::validateRefreshToken(rToken);

        if (!user.empty()) {
            auto newAccessToken = jwt::create()
                .set_issuer("crow_qt_server")
                .set_type("JWS")
                .set_payload_claim("username", jwt::claim(user))
                .set_issued_at(std::chrono::system_clock::now())
                .set_expires_at(std::chrono::system_clock::now() + std::chrono::minutes{15})
                .sign(jwt::algorithm::hs256{utils::getJwtSecret()});

            crow::json::wvalue resp;
            resp["token"] = newAccessToken;
            return crow::response(resp);
        }
        return crow::response(401, "Invalid refresh token");
    });

    // --- WHO AM I? (Check Auth & Status) ---
    CROW_ROUTE(app, "/api/auth/me")
        .methods(crow::HTTPMethod::GET)
        .CROW_MIDDLEWARES(app, AuthMiddleware)
    ([appPtr](const crow::request& req) {
        
        // HIER WAR DER FEHLER: Wir brauchen 'template', da appPtr ein Pointer auf ein Template ist
        auto& ctx = appPtr->template get_context<AuthMiddleware>(req);
        
        // User anhand des Tokens (Username) laden
        UserData u = DbManager::getUserByUsername(ctx.current_user);
        
        if (u.id == 0) return crow::response(404, "User not found");

        crow::json::wvalue result;
        result["id"] = u.id;
        result["username"] = u.username;
        result["is_active"] = u.isActive;
        result["force_password_change"] = u.forcePasswordChange;
        
        return crow::response(result);
    });

    // --- CHANGE MY PASSWORD ---
    CROW_ROUTE(app, "/api/user/change-password")
        .methods(crow::HTTPMethod::POST)
        .CROW_MIDDLEWARES(app, AuthMiddleware)
    ([appPtr](const crow::request& req) {
        
        // HIER EBENFALLS FIXEN
        auto& ctx = appPtr->template get_context<AuthMiddleware>(req);
        
        auto json = crow::json::load(req.body);

        if (!json || !json.has("oldPassword") || !json.has("newPassword")) {
            return crow::response(400, "Missing data");
        }

        std::string oldPass = json["oldPassword"].s();
        std::string newPass = json["newPassword"].s();

        if (newPass.length() < 8) return crow::response(400, "New password too short");

        // Aufruf DbManager
        int resCode = DbManager::changeOwnPassword(ctx.current_user, oldPass, newPass);

        if (resCode == 0) {
            return crow::response(200, "Password changed successfully");
        } else if (resCode == 2) {
            return crow::response(401, "Old password incorrect"); 
        } else {
            return crow::response(500, "Database error");
        }
    });
}

}