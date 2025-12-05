#pragma once

#include <string>
#include <QSqlDatabase>
#include <QString>
#include <QDateTime>
#include "metadata_extractor.hpp"

// Struktur für den Insert
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
    // Initialisiert die SQLite User-DB (Tabellen erstellen, Admin user)
    static void initAuthDatabase();

    // Liefert eine NEUE Verbindung zur PostgreSQL Datenbank (für Galerie/Uploads)
    static QSqlDatabase getPostgresConnection(const QString& connectionName);

    // Authentifizierung (SQLite)
    static bool verifyUser(const std::string& username, const std::string& password);

    // Token Management (SQLite)
    static void storeRefreshToken(const std::string& username, const std::string& token);
    static std::string validateRefreshToken(const std::string& token);
    static void revokeRefreshToken(const std::string& token);

    // Worker
    static bool insertPhoto(const WorkerPayload& p);

private:
    // Helper für interne SQLite Verbindungen
    static QSqlDatabase getAuthDbConnection(const QString& connectionName);

    static int getOrCreateKeywordId(QSqlDatabase& db, const QString& tag);
    
    static const QString SQLITE_DB_FILENAME;
};