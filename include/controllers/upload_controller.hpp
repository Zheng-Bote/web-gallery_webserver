#pragma once

#include "crow.h"
#include "crow/middlewares/cors.h"
#include "auth_middleware.hpp"

namespace routes {
    void setupUploadRoutes(crow::App<crow::CORSHandler, AuthMiddleware>& app);
}