#include <QCoreApplication>
#include <QThread>
#include <QDebug>
#include <thread>
#include <format>


#include "crow.h"
#include "crow/middlewares/cors.h"

// Unsere neuen Includes
#include "db_manager.hpp"
#include "auth_middleware.hpp"
#include "controllers/auth_controller.hpp"
#include "controllers/upload_controller.hpp"
#include "controllers/gallery_controller.hpp"
#include "controllers/web_controller.hpp" 

// Konfiguration (kannst du auch in config.hpp auslagern)
const int PORT = 8080;

void runCrowServer() {
    // App mit Middleware definieren
    crow::App<crow::CORSHandler, AuthMiddleware> app;

    // CORS Setup
    auto& cors = app.get_middleware<crow::CORSHandler>();
    cors.global()
        .headers("Content-Type", "Authorization")
        .methods("POST"_method, "GET"_method, "OPTIONS"_method)
        .origin("*");

    // --- ROUTEN REGISTRIEREN ---
    routes::setupAuthRoutes(app);
    routes::setupUploadRoutes(app);
    routes::setupGalleryRoutes(app);
    routes::setupWebRoutes(app);

    qInfo() << "Server starting on port" << PORT;
    app.port(PORT).multithreaded().run();
}

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    // 1. Datenbank initialisieren (Tabellen erstellen)
    DbManager::initAuthDatabase();

    // 2. Server in eigenem Thread starten
    std::jthread serverThread(runCrowServer);

    return app.exec();
}