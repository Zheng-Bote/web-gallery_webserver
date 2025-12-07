#pragma once

#include <string>
#include <QSqlDatabase>
#include <QString>
#include <QDateTime>
#include "metadata_extractor.hpp"

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