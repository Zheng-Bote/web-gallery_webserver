# CrowQtServer - High-Performance Backend

![C++](https://img.shields.io/badge/C++-20-blue.svg?logo=c%2B%2B)
![Qt](https://img.shields.io/badge/Qt-6.x-green.svg?logo=qt)
![Crow](https://img.shields.io/badge/Crow-v1.0-orange.svg)
![License](https://img.shields.io/badge/license-MIT-lightgrey.svg)

**CrowQtServer** is a high-performance, multithreaded REST API designed for the CrowGallery application. It leverages the speed of the **Crow Microframework** for asynchronous HTTP routing while utilizing the robustness of **Qt 6** for database management, file system operations, and string handling.

This server acts as the secure core of the application, handling JWT authentication, hybrid database interactions (SQLite & PostgreSQL), and image metadata processing.

---

## 🚀 Features

### 🔐 Security & Authentication
* **JWT Implementation:** Uses `jwt-cpp` to issue short-lived **Access Tokens** and secure, long-lived **Refresh Tokens**.
* **Bcrypt Hashing:** Passwords are salted and hashed using `libbcrypt` before storage.
* **Middleware Architecture:**
    * **AuthMiddleware:** Validates Bearer tokens before requests reach protected controllers.
    * **CORSHandler:** Custom implementation to handle Preflight (`OPTIONS`) requests correctly.
* **Role-Based Protection:** specific routes (e.g., User Management) are locked to the `admin` role.

### 🗄️ Hybrid Database System
* **SQLite (Authentication):** Fast, file-based storage for user credentials, roles, and tokens (`app_database.sqlite`).
* **PostgreSQL (Gallery Data):** Scalable, relational storage for millions of photos, Exif/IPTC metadata, and keywords.
* **Thread-Safe Pooling:** Implements a custom **Thread-Local Connection Pool**. Since Crow is multithreaded and `QSqlDatabase` is not thread-safe, the server creates a unique, named database connection for every active thread.

### ⚙️ User Management
* **CRUD Operations:** Admins can Create, Delete, and Deactivate users.
* **Security Policies:**
    * **Force Password Change:** Logic to force users to reset credentials upon next login (e.g., after an admin reset).
    * **Self-Service:** Endpoints for users to change their own passwords securely.

### 🖼️ Image Processing
* **Metadata Extraction:** Parsers for Exif, IPTC, and XMP data.
* **Smart Uploads:** Transaction-safe ingestion of new images into the PostgreSQL database.

---

## 🏗️ Architecture

The project follows a **Controller-Service-Repository** pattern adapted for modern C++.

### System Architecture

```mermaid
graph TD
    Client[Angular Frontend] -->|JSON / HTTP| Crow[Crow Router]
    
    subgraph "Middleware Layer"
        Crow --> CORS[CORS Handler]
        CORS --> AuthMW[Auth Middleware]
    end
    
    subgraph "Controller Layer"
        AuthMW --> AuthCtrl[Auth Controller]
        AuthMW --> AdminCtrl[Admin Controller]
        AuthMW --> GalleryCtrl[Gallery Controller]
        AuthMW --> UploadCtrl[Upload Controller]
    end
    
    subgraph "Data Access Layer (DbManager)"
        AuthCtrl --> SqliteConn[SQLite Connection]
        AdminCtrl --> SqliteConn
        GalleryCtrl --> PgConn[Postgres Connection]
        UploadCtrl --> PgConn
    end
    
    subgraph "Persistence"
        SqliteConn -->|Auth Data| File[(app_database.sqlite)]
        PgConn -->|Photo Data| PG[(PostgreSQL DB)]
    end
```

## Key Technical DecisionsQt 
+ Crow Hybrid:
  - We use Crow for the Web Server because it is significantly faster and lighter than Qt's QHttpServer.
  - We use Qt Core/Sql because its database drivers and string manipulation (QString) are superior to standard C++ for business logic.
    
  -- Concurrency Model:
   - Crow spawns multiple worker threads.
   - The DbManager uses QThread::currentThreadId() to assign specific database connections to specific threads. This ensures lock-free, thread-safe database access without race conditions.

    ## 🛠️ Tech Stack

  Language:    C++ 20

  Web Framework: CrowCore

  Framework: Qt 6.x (Modules: Core, Sql, Network)

  Dependencies:nlohmann/json (JSON Serialization)jwt-cpp (Token Generation/Validation)libbcrypt (Password Hashing)dotenv (Environment Configuration)

  # ⚙️ Installation & SetupPrerequisites

  C++ Compiler (GCC 10+, Clang 11+, or MSVC 2019+)

  CMake (3.14 or higher)

  Qt 6 SDK (Ensure Qt6Sql and Qt6Network are installed)

  PostgreSQL Server (running locally or remotely)

<pre>
Key Technical DecisionsQt + Crow Hybrid:We use Crow for the Web Server because it is significantly faster and lighter than Qt's QHttpServer.We use Qt Core/Sql because its database drivers and string manipulation (QString) are superior to standard C++ for business logic.Concurrency Model:Crow spawns multiple worker threads.The DbManager uses QThread::currentThreadId() to assign specific database connections to specific threads. This ensures lock-free, thread-safe database access without race conditions.🛠️ Tech StackLanguage: C++ 20Web Framework: CrowCore Framework: Qt 6.x (Modules: Core, Sql, Network)Dependencies:nlohmann/json (JSON Serialization)jwt-cpp (Token Generation/Validation)libbcrypt (Password Hashing)dotenv (Environment Configuration)⚙️ Installation & SetupPrerequisitesC++ Compiler (GCC 10+, Clang 11+, or MSVC 2019+)CMake (3.14 or higher)Qt 6 SDK (Ensure Qt6Sql and Qt6Network are installed)PostgreSQL Server (running locally or remotely)1. Clone the RepositoryBashgit clone [https://github.com/yourusername/crow-qt-server.git](https://github.com/yourusername/crow-qt-server.git)
cd crow-qt-server
2. Configure Environment (.env)Create a .env file in your root (or build) directory to configure the server:Ini, TOMLPORT=8080
JWT_SECRET=CHANGE_THIS_TO_A_LONG_RANDOM_STRING

# PostgreSQL Configuration
PG_HOST=localhost
PG_PORT=5432
PG_DB=Photos
PG_USER=postgres
PG_PASS=your_password
3. Build the ProjectBashmkdir build
cd build
cmake ..
make -j4
4. Run the ServerBash./CrowQtServer
Note: On the very first run, the server will automatically create the app_database.sqlite file and generate a default Admin User:Username: adminPassword: secretPlease change this password immediately via the API or Frontend.🔌 API Endpoints (Overview)MethodEndpointDescriptionAccessPOST/loginAuthenticate and retrieve tokensPublicPOST/refreshGet new Access Token via Refresh TokenPublicGET/api/auth/meGet current user status & flagsUserPOST/api/user/change-passwordChange own passwordUserGET/api/admin/usersList all registered usersAdminPOST/api/admin/usersCreate a new userAdminPUT/api/admin/users/:id/statusLock/Unlock a user accountAdminPOST/api/admin/users/:id/reset-passwordForce-reset a user's passwordAdmin
</pre>

## Project Structure

```bash
src/
├── controllers/       # Route definitions (Auth, Admin, Upload)
├── middlewares/       # Request pre-processing (Auth, CORS)
├── services/          # Business logic layer
├── main.cpp           # Entry point & Server configuration
├── db_manager.cpp     # QtSQL wrapper & Connection Pooling
├── db_manager.hpp     # Database Interface
├── utils.hpp          # Helper functions
└── CMakeLists.txt     # Build configuration
```


