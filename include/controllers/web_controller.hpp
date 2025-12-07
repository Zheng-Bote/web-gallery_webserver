#pragma once

#include "crow.h"
#include "crow/middlewares/cors.h"
#include "auth_middleware.hpp"

/**
 * @brief Application routes namespace.
 */
namespace routes {
    /**
     * @brief Handles static pages, templates and system info.
     * 
     * @param app The Crow application instance.
     */
    void setupWebRoutes(crow::App<crow::CORSHandler, AuthMiddleware>& app);
}