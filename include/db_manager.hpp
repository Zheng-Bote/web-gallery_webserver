#pragma once

#include <string>
#include <QSqlDatabase>
#include <QString>
#include <QDateTime>
#include "metadata_extractor.hpp"

// Structure for the Insert
struct WorkerPayload {
    std::string filename;
    std::string relPath;
    std::string fullPath;
    std::string user;
    long long fileSize;
    QDateTime fileDate;
    PhotoData meta;
};

class DbManager {
public:
    // Initializes the SQLite User-DB (create tables, admin user)
    static void initAuthDatabase();

    // Returns a NEW connection to the PostgreSQL database (for Gallery/Uploads)
    static QSqlDatabase getPostgresConnection();

    // Authentication (SQLite)
    static bool verifyUser(const std::string& username, const std::string& password);

    // Token Management (SQLite)
    static void storeRefreshToken(const std::string& username, const std::string& token);
    static std::string validateRefreshToken(const std::string& token);
    static void revokeRefreshToken(const std::string& token);

    // Worker
    static bool insertPhoto(const WorkerPayload& p);

private:
    // Helper for internal SQLite connections
    static QSqlDatabase getAuthDbConnection(const QString& connectionName);
    static int getOrCreateKeywordId(QSqlDatabase& db, const QString& tag);
    
    static const QString SQLITE_DB_FILENAME;
};