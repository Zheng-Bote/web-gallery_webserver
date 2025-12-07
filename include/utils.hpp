#pragma once
#include <string>

namespace utils {

    // Checks file extensions (jpg, png, etc.)
    bool isAllowedImage(const std::string& filename);

    // Generates random string (for Refresh Tokens)
    std::string generateRandomString(size_t length);

    // Gets the JWT Secret from ENV or uses fallback
    std::string getJwtSecret();

}