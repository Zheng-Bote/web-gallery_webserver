#pragma once

#include "crow.h"
#include "crow/middlewares/cors.h"
#include "auth_middleware.hpp"

namespace routes {
    // Kümmert sich um statische Seiten, Templates und System-Infos
    void setupWebRoutes(crow::App<crow::CORSHandler, AuthMiddleware>& app);
}