/**
 * @file utils.cpp
 * @brief Parameter implementations.
 */
#include "utils.hpp"

#include <set>
#include <algorithm>
#include <vector>
#include <cstdlib> // std::getenv
#include <stdexcept>
#include <openssl/rand.h>

namespace utils {

    bool isAllowedImage(const std::string& filename) {
        const std::set<std::string> allowed = { ".png", ".jpg", ".jpeg", ".bmp", ".tiff", ".gif" };
        
        size_t pos = filename.rfind('.');
        if (pos == std::string::npos) return false;

        std::string ext = filename.substr(pos);
        std::transform(ext.begin(), ext.end(), ext.begin(), 
                    [](unsigned char c){ return std::tolower(c); });

        return allowed.find(ext) != allowed.end();
    }

    std::string generateRandomString(size_t length) {
        const char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
        const size_t charsetSize = sizeof(charset) - 1;

        std::string result;
        result.resize(length);

        std::vector<unsigned char> buffer(length);
        if (RAND_bytes(buffer.data(), static_cast<int>(length)) != 1) {
            throw std::runtime_error("OpenSSL RAND_bytes failed");
        }

        for (size_t i = 0; i < length; i++) {
            result[i] = charset[buffer[i] % charsetSize];
        }
        return result;
    }

    std::string getJwtSecret() {
        const char* env_secret = std::getenv("JWT_SECRET");
        return env_secret ? std::string(env_secret) : "mein_sehr_geheimes_secret_key_12345";
    }

}