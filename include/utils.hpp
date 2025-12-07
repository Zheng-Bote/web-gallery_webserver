#pragma once
#include <string>

/**
 * @brief Utility functions.
 */
namespace utils {

    /**
     * @brief Checks if a file has an allowed image extension.
     * 
     * @param filename The filename to check.
     * @return true if allowed, false otherwise.
     */
    bool isAllowedImage(const std::string& filename);

    /**
     * @brief Generates a random alphanumeric string.
     * 
     * @param length Length of the string to generate.
     * @return The generated random string.
     */
    std::string generateRandomString(size_t length);

    /**
     * @brief Retrieves the JWT Secret.
     * 
     * Tries to read JWT_SECRET from environment variables, otherwise returns a fallback.
     * 
     * @return The JWT secret key.
     */
    std::string getJwtSecret();

}