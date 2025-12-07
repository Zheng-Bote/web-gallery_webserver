#pragma once

#include "crow.h"
#include "crow/middlewares/cors.h"
#include "auth_middleware.hpp"

/**
 * @brief Application routes namespace.
 */
namespace routes {
    /**
     * @brief Registers Gallery related routes.
     * 
     * @param app The Crow application instance.
     */
    void setupGalleryRoutes(crow::App<crow::CORSHandler, AuthMiddleware>& app);
}