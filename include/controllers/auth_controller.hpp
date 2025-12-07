#pragma once

#include "crow.h"
#include "crow/middlewares/cors.h"
#include "auth_middleware.hpp"

namespace routes {
    // Registers Login, Logout and Refresh routes
    void setupAuthRoutes(crow::App<crow::CORSHandler, AuthMiddleware>& app);
}