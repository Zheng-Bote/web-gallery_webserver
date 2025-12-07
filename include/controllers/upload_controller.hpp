#pragma once

#include "crow.h"
#include "crow/middlewares/cors.h"
#include "auth_middleware.hpp"

/**
 * @brief Application routes namespace.
 */
namespace routes {
    /**
     * @brief Registers Upload related routes.
     * 
     * @param app The Crow application instance.
     */
    void setupUploadRoutes(crow::App<crow::CORSHandler, AuthMiddleware>& app);
}