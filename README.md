# CrowQtServer - High-Performance Backend

![C++](https://img.shields.io/badge/C++-20-blue.svg?logo=c%2B%2B)
![Qt](https://img.shields.io/badge/Qt-6.x-green.svg?logo=qt)
![Crow](https://img.shields.io/badge/Crow-v1.0-orange.svg)
![License](https://img.shields.io/badge/license-MIT-lightgrey.svg)

Der **CrowQtServer** ist ein leistungsstarkes, multithreaded REST-Backend für die CrowGallery. Er kombiniert die Geschwindigkeit des **Crow Microframeworks** für HTTP-Routing mit der Robustheit und dem Ökosystem von **Qt 6** (SQL, Core, Filesystem).

Dieses Backend dient als zentrale API für das Angular-Frontend und verwaltet Authentifizierung, Bildverarbeitung und Datenbankzugriffe.

---

## 🚀 Features

### 🔐 Sicherheit & Auth
* **JWT Authentifizierung:** Implementiert Access-Tokens (kurzlebig) und Refresh-Tokens (langlebig) mit `jwt-cpp`.
* **Sicheres Hashing:** Passwörter werden mittels **Bcrypt** und Salt gespeichert.
* **Middleware:** Vorgeschaltete Auth-Middleware zur Validierung von Tokens vor Controller-Zugriff.
* **CORS Support:** Vollständige Konfiguration für Cross-Origin Resource Sharing.

### 🗄️ Hybride Datenbank-Architektur
* **SQLite (Auth):** Schnelle, lokale Verwaltung von Benutzern, Passwörtern und Refresh-Tokens (`app_database.sqlite`).
* **PostgreSQL (Data):** Skalierbare Speicherung für Bilddaten, Metadaten (Exif, IPTC) und Keywords.
* **Connection Pooling:** Thread-safe Datenbankverbindungen über `QSqlDatabase` (eine Verbindung pro Thread).

### ⚙️ Administration & User-Management
* **User Lifecycle:** Anlegen, Löschen und Deaktivieren (Sperren) von Benutzern.
* **Password Policy:** Unterstützung für erzwungene Passwortänderungen (Flag `force_password_change`) und Admin-Resets.
* **Role-Checks:** Spezielle Endpunkte, die nur für den `admin` User zugänglich sind.

### 🖼️ Medienverwaltung
* **Metadata Extraction:** Liest Exif, IPTC und XMP Daten aus Bildern aus.
* **Smart Upload:** Verarbeitet Uploads, generiert Pfade und speichert Metadaten transaktionssicher.
* **Optimierung:** (Vorbereitet) Generierung von Thumbnails und WebP-Varianten.

---

## 🏗️ Architektur

Das Projekt nutzt eine **Controller-Service-Repository** ähnliche Struktur, angepasst an C++.

### High-Level Übersicht

```mermaid
graph TD
    Client[Angular Frontend] -->|HTTP Request| Crow[Crow Router]
    
    subgraph "Middleware Layer"
        Crow --> CORS[CORS Handler]
        CORS --> AuthMW[Auth Middleware]
    end
    
    subgraph "Controller Layer"
        AuthMW --> AuthCtrl[Auth Controller]
        AuthMW --> AdminCtrl[Admin Controller]
        AuthMW --> GalleryCtrl[Gallery Controller]
    end
    
    subgraph "Service / Data Layer"
        AuthCtrl --> DbManager
        AdminCtrl --> DbManager
        GalleryCtrl --> DbManager
    end
    
    subgraph "Persistence"
        DbManager -->|QtSQL| SQLite[(SQLite: Users)]
        DbManager -->|QtSQL| Postgres[(PostgreSQL: Photos)]
<!-- readme-tree start -->
```
.
├── .github
│   ├── actions
│   │   └── doctoc
│   │       ├── README.md
│   │       ├── action.yml
│   │       └── dist
│   │           ├── index.js
│   │           ├── index.js.map
│   │           ├── licenses.txt
│   │           └── sourcemap-register.js
│   └── workflows
│       ├── ghp-call_Readme.yml
│       ├── ghp-create_doctoc.yml
│       ├── ghp-markdown_index.yml
│       ├── repo-actions_docu.yml
│       ├── repo-call_Readme.yml
│       ├── repo-create_doctoc.yml_
│       ├── repo-create_doctoc_md.yml
│       └── repo-create_tree_readme.yml
├── .gitignore
├── CMakeLists.txt
├── LICENSE
├── README.md
├── build_appimage.sh
├── configure
│   ├── CMakeLists.txt
│   └── rz_config.hpp.in
├── crowqt.desktop
├── crowqt.png
├── docs
│   ├── deployment.md
│   └── todos.md
├── include
│   ├── auth_middleware.hpp
│   ├── controllers
│   │   ├── admin_controller.hpp
│   │   ├── auth_controller.hpp
│   │   ├── gallery_controller.hpp
│   │   ├── upload_controller.hpp
│   │   └── web_controller.hpp
│   ├── db_manager.hpp
│   ├── image_processor.hpp
│   ├── metadata_extractor.hpp
│   ├── rz_config.hpp
│   └── utils.hpp
├── src
│   ├── auth_middleware.cpp
│   ├── controllers
│   │   ├── admin_controller.cpp
│   │   ├── auth_controller.cpp
│   │   ├── gallery_controller.cpp
│   │   ├── upload_controller.cpp
│   │   └── web_controller.cpp
│   ├── db_manager.cpp
│   ├── image_processor.cpp
│   ├── main.cpp
│   ├── metadata_extractor.cpp
│   └── utils.cpp
├── static
│   └── index.html
├── templates
│   └── template.html
└── tree.bak

14 directories, 50 files
```
<!-- readme-tree end -->
