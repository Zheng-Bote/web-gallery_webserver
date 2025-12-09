#pragma once
#include "crow.h"
#include "crow/middlewares/cors.h" // Wichtig
#include "auth_middleware.hpp"

namespace routes {
    void setupAdminRoutes(crow::App<crow::CORSHandler, AuthMiddleware>& app);
}
