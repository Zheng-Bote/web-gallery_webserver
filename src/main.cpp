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

#include "db_manager.hpp"
#include "auth_middleware.hpp"
#include "controllers/auth_controller.hpp"
#include "controllers/upload_controller.hpp"
#include "controllers/gallery_controller.hpp"
#include "controllers/web_controller.hpp" 

// Port optionally loaded from ENV
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
    // /path/to/CrowQtServer.env
   // --- PATH LOGIC ---
    QString envPath;
    
    // Check if we are running inside an AppImage
    // The variable APPIMAGE contains the path to the .AppImage file itself
    const char* appImageEnv = std::getenv("APPIMAGE");
    
    if (appImageEnv) {
        // We are an AppImage!
        // Config sits NEXT TO the AppImage File (not inside it)
        QFileInfo appImageInfo(appImageEnv);
        envPath = appImageInfo.absoluteDir().filePath("CrowQtServer.env");
        
        // Also set the Working Directory so 'Photos/' ends up next to the AppImage
        QDir::setCurrent(appImageInfo.absolutePath());
    } else {
        // Normal Dev-Mode
        envPath = QCoreApplication::applicationFilePath() + ".env";
    }

    // --- LOAD DOTENV ---    
    // Check if file exists (to avoid unnecessary exception in log)
    if (QFile::exists(envPath)) {
        try {
            // dotenv loads values into std::getenv
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