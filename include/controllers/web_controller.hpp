#pragma once

#include "crow.h"
#include "crow/middlewares/cors.h"
#include "auth_middleware.hpp"

namespace routes {
    // Handles static pages, templates and system info
    void setupWebRoutes(crow::App<crow::CORSHandler, AuthMiddleware>& app);
}