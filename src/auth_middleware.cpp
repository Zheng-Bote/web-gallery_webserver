#include "auth_middleware.hpp"
#include "utils.hpp" // Für getJwtSecret

#include <jwt-cpp/jwt.h>

const std::string JWT_ISSUER = "crow_qt_server";

void AuthMiddleware::before_handle(crow::request& req, crow::response& res, context& ctx) {

    // -----------------------------------------------------------------------
    // FIX FÜR CORS: OPTIONS Requests immer durchlassen!
    // Browser senden OPTIONS ohne Token. Das darf nicht 401 sein.
    // -----------------------------------------------------------------------
    if (req.method == crow::HTTPMethod::OPTIONS) {
        // Wir machen hier nichts und lassen Crow (CORS Handler) weitermachen.
        // Ein return hier beendet die Middleware-Kette für diesen Request nicht als Fehler.
        return; 
    }
    
    // 1. Authorization Header holen
    std::string authHeader = req.get_header_value("Authorization");
    
    // Immer JSON im Fehlerfall
    res.set_header("Content-Type", "application/json");

    if (authHeader.empty()) {
        res.code = 401;
        res.end(R"({"error": "Missing Authorization Header"})");
        return;
    }

    if (authHeader.substr(0, 7) != "Bearer ") {
        res.code = 401;
        res.end(R"({"error": "Invalid Authorization Header format. Expected 'Bearer <token>'"})");
        return;
    }

    std::string token = authHeader.substr(7);

    try {
        auto decoded = jwt::decode(token);
        
        auto verifier = jwt::verify()
            .allow_algorithm(jwt::algorithm::hs256{ utils::getJwtSecret() })
            .with_issuer(JWT_ISSUER);

        verifier.verify(decoded);

        if (decoded.has_payload_claim("username")) {
            ctx.current_user = decoded.get_payload_claim("username").as_string();
            ctx.is_authenticated = true;
        }

    } catch (const std::exception& e) {
        res.code = 401;
        std::string msg = e.what();
        // Simples JSON escaping
        res.end("{\"error\": \"Token verification failed\", \"details\": \"" + msg + "\"}");
    }
}