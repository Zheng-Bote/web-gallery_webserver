#pragma once
#include "crow.h"
#include <string>

struct AuthMiddleware : crow::ILocalMiddleware {
    struct context {
        std::string current_user;
        bool is_authenticated = false;
    };

    void before_handle(crow::request& req, crow::response& res, context& ctx);

    void after_handle(crow::request& /*req*/, crow::response& /*res*/, context& /*ctx*/) {
        // Nothing to do
    }
};
