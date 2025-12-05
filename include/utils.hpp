#pragma once
#include <string>

namespace utils {

    // Prüft Dateiendungen (jpg, png, etc.)
    bool isAllowedImage(const std::string& filename);

    // Generiert Zufallsstring (für Refresh Tokens)
    std::string generateRandomString(size_t length);

    // Holt das JWT Secret aus ENV oder nutzt Fallback
    std::string getJwtSecret();

}