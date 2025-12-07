#pragma once

#include "crow.h"
#include "crow/middlewares/cors.h"
#include "auth_middleware.hpp"

/**
 * @brief Application routes namespace.
 */
namespace routes {
    /**
     * @brief Registers Login, Logout and Refresh routes.
     * 
     * @param app The Crow application instance.
     */
    void setupAuthRoutes(crow::App<crow::CORSHandler, AuthMiddleware>& app);
}