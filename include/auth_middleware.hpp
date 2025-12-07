#pragma once
#include "crow.h"
#include <string>

/**
 * @brief Middleware for handling Authentication.
 * 
 * Verifies JWT tokens and manages user context.
 */
struct AuthMiddleware : crow::ILocalMiddleware {
    /**
     * @brief Context structure for the middleware.
     */
    struct context {
        std::string current_user; ///< The username of the authenticated user.
        bool is_authenticated = false; ///< Authentication status.
    };

    /**
     * @brief Executed before the request is handled.
     * 
     * @param req The incoming request.
     * @param res The response to be sent (if error occurs).
     * @param ctx The middleware context.
     */
    void before_handle(crow::request& req, crow::response& res, context& ctx);

    /**
     * @brief Executed after the request is handled.
     * 
     * @param req The request.
     * @param res The response.
     * @param ctx The middleware context.
     */
    void after_handle(crow::request& /*req*/, crow::response& /*res*/, context& /*ctx*/) {
        // Nothing to do
    }
};
