#include "db_manager.hpp"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QThread>
#include <QDateTime>
#include <QProcessEnvironment>
#include <QVariant>
#include <QSqlDriver> // For Transaction-Checks
#include "bcrypt/BCrypt.hpp"

const QString DbManager::SQLITE_DB_FILENAME = "app_database.sqlite";

// ------------------------------------------------------------------
// POSTGRESQL (Data / Gallery) - WITH CONNECTION POOLING
// ------------------------------------------------------------------
QSqlDatabase DbManager::getPostgresConnection() {
    // Name based on Thread-ID -> Each Thread has its own, persistent connection
    QString connName = QString("pg_conn_%1").arg((quint64)QThread::currentThreadId());

    if (QSqlDatabase::contains(connName)) {
        QSqlDatabase db = QSqlDatabase::database(connName);
        
        if (db.isOpen()) {
            return db;
        } else {
            // Try to reopen (in case of Timeout etc.)
            if (!db.open()) {
                qCritical() << "Failed to reopen existing Postgres connection:" << connName << db.lastError().text();
            }
            return db;
        }
    }

    // Create new connection (only 1x per Thread)
    QSqlDatabase db = QSqlDatabase::addDatabase("QPSQL", connName);
    
    db.setHostName(qEnvironmentVariable("PG_HOST", "localhost"));
    db.setPort(qEnvironmentVariable("PG_PORT", "5432").toInt());
    db.setDatabaseName(qEnvironmentVariable("PG_DB", "Photos"));
    db.setUserName(qEnvironmentVariable("PG_USER", "postgres"));
    db.setPassword(qEnvironmentVariable("PG_PASS"));

    if (!db.open()) {
        qCritical() << "Postgres Connection Error (" << connName << "):" << db.lastError().text();
    } else {
        qDebug() << "New Postgres Connection established for thread:" << connName;
    }

    return db;
}

// ------------------------------------------------------------------
// SQLITE (Auth / Users)
// ------------------------------------------------------------------

QSqlDatabase DbManager::getAuthDbConnection(const QString& connectionName) {
    QSqlDatabase db;
    if (QSqlDatabase::contains(connectionName)) {
        db = QSqlDatabase::database(connectionName);
    } else {
        db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
        db.setDatabaseName(SQLITE_DB_FILENAME);
    }

    if (!db.isOpen() && !db.open()) {
        qCritical() << "Auth DB Open Error:" << db.lastError().text();
    }
    return db;
}

void DbManager::initAuthDatabase() {
    {
        QSqlDatabase db = getAuthDbConnection("setup_conn");
        if (!db.isOpen()) return;

        QSqlQuery query(db);
        query.exec("CREATE TABLE IF NOT EXISTS users (id INTEGER PRIMARY KEY AUTOINCREMENT, username TEXT UNIQUE NOT NULL, password_hash TEXT NOT NULL, created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP)");
        query.exec("CREATE TABLE IF NOT EXISTS refresh_tokens (token TEXT PRIMARY KEY, username TEXT, expires_at INTEGER)");

        query.exec("SELECT count(*) FROM users WHERE username = 'admin'");
        if (query.next() && query.value(0).toInt() == 0) {
            std::string hash = BCrypt::generateHash("secret");
            QSqlQuery insert(db);
            insert.prepare("INSERT INTO users (username, password_hash) VALUES (:u, :p)");
            insert.bindValue(":u", "admin");
            insert.bindValue(":p", QString::fromStdString(hash));
            insert.exec();
            qDebug() << "Initial Admin user created (User: admin, Pass: secret)";
        }
    }
    QSqlDatabase::removeDatabase("setup_conn");
}

bool DbManager::verifyUser(const std::string& username, const std::string& password) {
    QString connName = QString("auth_verify_%1").arg((quint64)QThread::currentThreadId());
    bool isValid = false;
    {
        QSqlDatabase db = getAuthDbConnection(connName);
        if (db.isOpen()) {
            QSqlQuery query(db);
            query.prepare("SELECT password_hash FROM users WHERE username = :u");
            query.bindValue(":u", QString::fromStdString(username));
            if (query.exec() && query.next()) {
                std::string storedHash = query.value(0).toString().toStdString();
                isValid = BCrypt::validatePassword(password, storedHash);
            }
        }
    }
    QSqlDatabase::removeDatabase(connName);
    return isValid;
}

void DbManager::storeRefreshToken(const std::string& username, const std::string& token) {
    QString connName = QString("auth_store_%1").arg((quint64)QThread::currentThreadId());
    {
        QSqlDatabase db = getAuthDbConnection(connName);
        if(db.isOpen()) {
            QSqlQuery q(db);
            q.prepare("DELETE FROM refresh_tokens WHERE username = :u");
            q.bindValue(":u", QString::fromStdString(username));
            q.exec();

            qint64 expiry = QDateTime::currentSecsSinceEpoch() + (7 * 24 * 60 * 60);
            q.prepare("INSERT INTO refresh_tokens (token, username, expires_at) VALUES (:t, :u, :e)");
            q.bindValue(":t", QString::fromStdString(token));
            q.bindValue(":u", QString::fromStdString(username));
            q.bindValue(":e", expiry);
            q.exec();
        }
    }
    QSqlDatabase::removeDatabase(connName);
}

std::string DbManager::validateRefreshToken(const std::string& token) {
    QString connName = QString("auth_val_%1").arg((quint64)QThread::currentThreadId());
    std::string username = "";
    {
        QSqlDatabase db = getAuthDbConnection(connName);
        if(db.isOpen()) {
            QSqlQuery q(db);
            q.prepare("SELECT username, expires_at FROM refresh_tokens WHERE token = :t");
            q.bindValue(":t", QString::fromStdString(token));
            if (q.exec() && q.next()) {
                qint64 exp = q.value("expires_at").toLongLong();
                if (QDateTime::currentSecsSinceEpoch() < exp) {
                    username = q.value("username").toString().toStdString();
                } else {
                    QSqlQuery del(db);
                    del.prepare("DELETE FROM refresh_tokens WHERE token = :t");
                    del.bindValue(":t", QString::fromStdString(token));
                    del.exec();
                }
            }
        }
    }
    QSqlDatabase::removeDatabase(connName);
    return username;
}

void DbManager::revokeRefreshToken(const std::string& token) {
    QString connName = QString("auth_revoke_%1").arg((quint64)QThread::currentThreadId());
    {
        QSqlDatabase db = getAuthDbConnection(connName);
        if (db.isOpen()) {
            QSqlQuery q(db);
            q.prepare("DELETE FROM refresh_tokens WHERE token = :t");
            q.bindValue(":t", QString::fromStdString(token));
            q.exec();
        }
    }
    QSqlDatabase::removeDatabase(connName);
}

// ------------------------------------------------------------------
// WORKER / UPLOAD LOGIC
// ------------------------------------------------------------------

int DbManager::getOrCreateKeywordId(QSqlDatabase& db, const QString& tag) {
    QSqlQuery q(db);
    q.prepare("SELECT id FROM keywords WHERE tag = :t");
    q.bindValue(":t", tag);
    if (q.exec() && q.next()) return q.value(0).toInt();

    q.prepare("INSERT INTO keywords (tag) VALUES (:t) ON CONFLICT (tag) DO NOTHING RETURNING id");
    q.bindValue(":t", tag);
    if (q.exec() && q.next()) return q.value(0).toInt();

    q.prepare("SELECT id FROM keywords WHERE tag = :t");
    q.bindValue(":t", tag);
    if (q.exec() && q.next()) return q.value(0).toInt();

    return -1;
}

bool DbManager::insertPhoto(const WorkerPayload& p) {
    // NEW: Use pooled connection (no UUID anymore)
    QSqlDatabase db = getPostgresConnection();
    
    if (!db.isOpen()) return false;

    // Start Transaction
    db.transaction();

    bool ok = true;
    qint64 picId = -1;

    // 1. Picture
    QSqlQuery q(db);
    q.prepare("INSERT INTO pictures (file_name, file_path, full_path, file_size, width, height, file_datetime, upload_user) "
              "VALUES (:fn, :fp, :full, :sz, :w, :h, :dt, :usr) RETURNING id");
    q.bindValue(":fn", QString::fromStdString(p.filename));
    q.bindValue(":fp", QString::fromStdString(p.relPath));
    q.bindValue(":full", QString::fromStdString(p.fullPath));
    q.bindValue(":sz", p.fileSize);
    q.bindValue(":w", p.meta.width);
    q.bindValue(":h", p.meta.height);
    q.bindValue(":dt", p.fileDate);
    q.bindValue(":usr", QString::fromStdString(p.user));
    
    if (q.exec() && q.next()) {
        picId = q.value(0).toLongLong();
    } else {
        ok = false;
        qCritical() << "Insert Picture failed:" << q.lastError().text();
    }

    if (ok) {
        // 2. Location
        QSqlQuery qLoc(db);
        qLoc.prepare("INSERT INTO meta_location (ref_picture, country, country_code, province, city) VALUES (:id, :c, :cc, :p, :ci)");
        qLoc.bindValue(":id", picId);
        qLoc.bindValue(":c", p.meta.country);
        qLoc.bindValue(":cc", p.meta.countryCode);
        qLoc.bindValue(":p", p.meta.province);
        qLoc.bindValue(":ci", p.meta.city);
        if(!qLoc.exec()) {
             qWarning() << "Insert Location failed:" << qLoc.lastError().text();
             // Not critical
        }

        // 3. Exif
        QSqlQuery qExif(db);
        qExif.prepare("INSERT INTO meta_exif (ref_picture, make, model, iso, aperture, exposure_time, gps_latitude, gps_longitude, datetime_original) "
                      "VALUES (:id, :mk, :md, :iso, :ap, :exp, :lat, :lon, :dto)");
        qExif.bindValue(":id", picId);
        qExif.bindValue(":mk", p.meta.make);
        qExif.bindValue(":md", p.meta.model);
        qExif.bindValue(":iso", p.meta.iso);
        qExif.bindValue(":ap", p.meta.aperture);
        qExif.bindValue(":exp", p.meta.exposure);
        qExif.bindValue(":lat", p.meta.gpsLat);
        qExif.bindValue(":lon", p.meta.gpsLon);
        qExif.bindValue(":dto", p.meta.takenAt);
        if(!qExif.exec()) {
            qWarning() << "Insert Exif failed:" << qExif.lastError().text();
            // Not critical
        }

        // 4. IPTC / XMP (Text Data)
        QSqlQuery qIptc(db);
        qIptc.prepare("INSERT INTO meta_iptc (ref_picture, object_name, caption, copyright) VALUES (:id, :obj, :cap, :copy)");
        qIptc.bindValue(":id", picId);
        qIptc.bindValue(":obj", p.meta.title);
        QString cap = p.meta.caption.isEmpty() ? p.meta.description : p.meta.caption;
        qIptc.bindValue(":cap", cap);
        qIptc.bindValue(":copy", p.meta.copyright);
        
        if(!qIptc.exec()) {
             qWarning() << "Insert IPTC failed:" << qIptc.lastError().text();
        }

        // 5. Keywords
        for(const auto& k : p.meta.keywords) {
            if(k.trimmed().isEmpty()) continue;
            int kId = getOrCreateKeywordId(db, k.trimmed());
            if(kId > 0) {
                QSqlQuery qLink(db);
                qLink.prepare("INSERT INTO picture_keywords (picture_id, keyword_id) VALUES (:pid, :kid) ON CONFLICT DO NOTHING");
                qLink.bindValue(":pid", picId);
                qLink.bindValue(":kid", kId);
                qLink.exec();
            }
        }
    }

    if (ok) {
        db.commit();
        qDebug() << "DB Insert success for ID:" << picId;
    } else {
        db.rollback();
    }

    // IMPORTANT: DO NOT close connection, it remains open for the Thread
    return ok; 
}