/**
 * @file main.cpp
 * @brief Entry point of the Crow Web-Gallery Server.
 */
#include <QCoreApplication>
#include <QThread>
#include <QDebug>
#include <QFile>
#include <thread>
#include <format>
#include <QFileInfo>
#include <QDir> 

#include "crow.h"
#include "crow/middlewares/cors.h"

#include "dotenv.h"

// Our new Includes
#include "db_manager.hpp"
#include "auth_middleware.hpp"
#include "controllers/auth_controller.hpp"
#include "controllers/upload_controller.hpp"
#include "controllers/gallery_controller.hpp"
#include "controllers/web_controller.hpp" 

// Port optional auch aus ENV laden
int getPort() {
    if (const char* env_p = std::getenv("PORT")) return std::atoi(env_p);
    return 8080;
}

void runCrowServer() {
    // Define App with Middleware
    crow::App<crow::CORSHandler, AuthMiddleware> app;

    // CORS Setup
    auto& cors = app.get_middleware<crow::CORSHandler>();
    cors.global()
        .headers("Content-Type", "Authorization")
       .methods("POST"_method, "GET"_method, "OPTIONS"_method, "DELETE"_method, "PUT"_method)
        .origin("*");

    // --- REGISTER ROUTES ---
    routes::setupAuthRoutes(app);
    routes::setupUploadRoutes(app);
    routes::setupGalleryRoutes(app);
    routes::setupWebRoutes(app);

    qInfo() << "Server starting on port" << getPort();
    app.port(getPort()).multithreaded().run();
}

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    // --- DOTENV SETUP ---
    // /pfad/zu/CrowQtServer.env
   // --- PFAD LOGIK ---
    QString envPath;
    
    // Prüfen, ob wir in einem AppImage laufen
    // Die Variable APPIMAGE enthält den Pfad zum .AppImage File selbst
    const char* appImageEnv = std::getenv("APPIMAGE");
    
    if (appImageEnv) {
        // Wir sind ein AppImage!
        // Config liegt NEBEN dem AppImage File (nicht darin)
        QFileInfo appImageInfo(appImageEnv);
        envPath = appImageInfo.absoluteDir().filePath("CrowQtServer.env");
        
        // Auch das Working Directory setzen, damit 'Photos/' neben dem AppImage landen
        QDir::setCurrent(appImageInfo.absolutePath());
    } else {
        // Normaler Dev-Mode
        envPath = QCoreApplication::applicationFilePath() + ".env";
    }

    // --- DOTENV LADEN ---    
    // Prüfen ob Datei existiert (um unnötige Exception im Log zu vermeiden)
    if (QFile::exists(envPath)) {
        try {
            // dotenv lädt die Werte in std::getenv
            dotenv::init(envPath.toStdString().c_str());
            qInfo() << "Environment loaded from:" << envPath;
        } catch (const std::exception& e) {
            qWarning() << "Failed to parse .env file:" << e.what();
        }
    } else {
        qInfo() << "No .env file found at:" << envPath << "(using system env or defaults)";
    }
    // --------------------

    // 1. Initialize Database (Create Tables)
    DbManager::initAuthDatabase();

    // 2. Start Server in its own Thread
    std::jthread serverThread(runCrowServer);

    return app.exec();
}