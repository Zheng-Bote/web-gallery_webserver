#pragma once

#include <string>
#include <QSqlDatabase>
#include <QString>
#include <QDateTime>
#include "metadata_extractor.hpp"
#include <QFile>

/**
 * @brief Structure for the Worker Payload.
 * 
 * Contains all necessary information for inserting a new photo into the database.
 */
struct WorkerPayload {
    std::string filename; ///< The original filename.
    std::string relPath; ///< Relative path to the file.
    std::string fullPath; ///< Full system path to the file.
    std::string user; ///< Username of the uploader.
    long long fileSize; ///< Size of the file in bytes.
    QDateTime fileDate; ///< Timestamp when the file was created/taken.
    PhotoData meta; ///< Extracted metadata.
};

/**
 * @brief Structure for Photo Updates.
 */
struct PhotoUpdateData {
    std::string title; ///< New title.
    std::string description; ///< New description (mapped to 'caption' in DB).
    std::vector<std::string> keywords; ///< New list of keywords.
};

/**
 * @brief Structure for User Data.
 * 
 */
struct UserData {
    int id;
    std::string username;
    std::string createdAt;
    bool isActive;
};

/**
 * @brief Database Manager Class.
 * 
 * Handles all interactions with SQLite (Auth) and PostgreSQL (Data) databases.
 */
class DbManager {
public:
    /**
     * @brief Initializes the SQLite User-DB (create tables, admin user).
     */
    static void initAuthDatabase();

    /**
     * @brief Returns a NEW connection to the PostgreSQL database (for Gallery/Uploads).
     * 
     * @return QSqlDatabase configured for PostgreSQL.
     */
    static QSqlDatabase getPostgresConnection();

    /**
     * @brief Verifies the user credentials.
     * 
     * @param username The username to verify.
     * @param password The password to verify.
     * @return true if credentials are valid, false otherwise.
     */
    static bool verifyUser(const std::string& username, const std::string& password);

    /**
     * @brief Stores a refresh token for a user.
     * 
     * @param username The username.
     * @param token The refresh token string.
     */
    static void storeRefreshToken(const std::string& username, const std::string& token);

    /**
     * @brief Validates a refresh token.
     * 
     * @param token The refresh token to validate.
     * @return The username associated with the token if valid, empty string otherwise.
     */
    static std::string validateRefreshToken(const std::string& token);

    /**
     * @brief Revokes (deletes) a refresh token.
     * 
     * @param token The token to revoke.
     */
    static void revokeRefreshToken(const std::string& token);

    /**
     * @brief Inserts a photo and its metadata into the database.
     * 
     * @param p The worker payload containing photo data.
     * @return true if insertion was successful, false otherwise.
     */
    static bool insertPhoto(const WorkerPayload& p);

    /**
     * @brief Updates a photo's metadata.
     * 
     * @param id The ID of the photo to update.
     * @param data The new metadata.
     * @return true if update was successful, false otherwise.
     */
    static bool updatePhotoMetadata(int id, const PhotoUpdateData& data);

    /**
     * @brief Deletes a photo from the database.
     * 
     * @param id The ID of the photo to delete.
     * @return true if deletion was successful, false otherwise.
     */
    static bool deletePhoto(int id);

    // User Management (SQLite)
    /**
     * @brief Gets all users from the database.
     * 
     * @return A vector of UserData objects.
     */
    static std::vector<UserData> getAllUsers();
    
    /**
     * @brief Creates a new user in the database.
     * 
     * @param username The username.
     * @param password The password.
     * @return true if creation was successful, false otherwise.
     */
    static bool createUser(const std::string& username, const std::string& password);
    /**
     * @brief Deletes a user from the database.
     * 
     * @param id The ID of the user to delete.
     * @return true if deletion was successful, false otherwise.
     */
    static bool deleteUser(int id);
    /**
     * @brief Changes a user's password.
     * 
     * @param id The ID of the user.
     * @param newPassword The new password.
     * @return true if password change was successful, false otherwise.
     */
     static bool changePassword(int id, const std::string& newPassword);
    /**
    * @brief Updates the status of a user.
    * 
    * @param id The ID of the user.
    * @param active The new status.
    * @return true if update was successful, false otherwise.
    */
    static bool updateUserStatus(int id, bool active);



private:
    /**
     * @brief Helper for internal SQLite connections.
     * 
     * @param connectionName Unique name for this connection.
     * @return QSqlDatabase configured for SQLite.
     */
    static QSqlDatabase getAuthDbConnection(const QString& connectionName);

    /**
     * @brief Gets or creates a keyword ID.
     * 
     * @param db The database connection.
     * @param tag The keyword tag.
     * @return The ID of the keyword.
     */
    static int getOrCreateKeywordId(QSqlDatabase& db, const QString& tag);
    
    static const QString SQLITE_DB_FILENAME; ///< Filename for the SQLite database.
};