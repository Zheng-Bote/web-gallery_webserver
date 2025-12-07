#include "auth_middleware.hpp"
#include "utils.hpp" // For getJwtSecret

#include <jwt-cpp/jwt.h>

const std::string JWT_ISSUER = "crow_qt_server";

void AuthMiddleware::before_handle(crow::request& req, crow::response& res, context& ctx) {

    // -----------------------------------------------------------------------
    // FIX FOR CORS: Always pass OPTIONS requests!
    // Browsers send OPTIONS without a Token. This must not be 401.
    // -----------------------------------------------------------------------
    if (req.method == crow::HTTPMethod::OPTIONS) {
        // We do nothing here and let Crow (CORS Handler) continue.
        // A return here does not end the middleware chain for this request as an error.
        return; 
    }
    
    // 1. Get Authorization Header
    std::string authHeader = req.get_header_value("Authorization");
    
    // Always JSON in case of error
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
        // Simple JSON escaping
        res.end("{\"error\": \"Token verification failed\", \"details\": \"" + msg + "\"}");
    }
}