#pragma once

#include "crow.h"
#include "crow/middlewares/cors.h"
#include "auth_middleware.hpp"

namespace routes {
    // Registers Gallery related routes
    void setupGalleryRoutes(crow::App<crow::CORSHandler, AuthMiddleware>& app);
}