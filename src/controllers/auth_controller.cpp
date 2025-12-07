#include "controllers/auth_controller.hpp"
#include "db_manager.hpp"
#include "utils.hpp"
#include <jwt-cpp/jwt.h>

namespace routes {

void setupAuthRoutes(crow::App<crow::CORSHandler, AuthMiddleware>& app) {
    
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
}

}