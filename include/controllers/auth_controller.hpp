#pragma once

#include "crow.h"
#include "crow/middlewares/cors.h"
#include "auth_middleware.hpp"

namespace routes {
    // Registriert Login, Logout und Refresh Routen
    void setupAuthRoutes(crow::App<crow::CORSHandler, AuthMiddleware>& app);
}