cmake_minimum_required(VERSION 3.23...3.30)

# cmake -S . -B build
# cmake --build build


project(CrowQtServer 
    VERSION 0.9.0
    DESCRIPTION "Crow Webserver with Qt6 & Worker"
    HOMEPAGE_URL "https://github.com/Zheng-Bote/web-gallery_webserver"
    LANGUAGES CXX
)

# --- Compiler Settings ---
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Enable warnings (Best Practice)
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra)
endif()


# --- Qt6 Setup ---
find_package(QT NAMES Qt6 REQUIRED COMPONENTS Core Sql Gui Concurrent)
find_package(Qt6 REQUIRED COMPONENTS Core Sql Gui Concurrent)
# Resolve Qt conflict (Crow vs Qt Signals)
add_compile_definitions(QT_NO_KEYWORDS)

# --- Configure ---
set(EXECUTABLE_NAME "${PROJECT_NAME}")
set(PROG_LONGNAME "Crow Web-Gallery Server")
set(PROG_CREATED "2025")
set(PROG_AUTHOR "ZHENG Bote")
set(PROG_ORGANIZATION_NAME "ZHENG Robert")
set(PROG_ORGANIZATION_DOMAIN "net.hase-zheng")
if(Qt6_FOUND)
    set(CMAKE_QTV "${Qt6_VERSION}")
else()
    set(CMAKE_QTV "Unknown")
endif()
add_subdirectory(configure)

# --- Dependencies via FetchContent ---
include(FetchContent)

# 1. ASIO (Important: BEFORE Crow)
FetchContent_Declare(
    asio
    GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
    GIT_TAG        asio-1-30-2
)
FetchContent_MakeAvailable(asio)
set(ASIO_INCLUDE_DIR ${asio_SOURCE_DIR}/asio/include)

# 2. Crow
FetchContent_Declare(
    crow
    GIT_REPOSITORY https://github.com/CrowCpp/Crow.git
    GIT_TAG        master
)
FetchContent_MakeAvailable(crow)

# 3. JWT-CPP
FetchContent_Declare(
    jwt-cpp
    GIT_REPOSITORY https://github.com/Thalhammer/jwt-cpp.git
    GIT_TAG        v0.7.0
)
FetchContent_MakeAvailable(jwt-cpp)

# 4. LibBcrypt
FetchContent_Declare(
    libbcrypt
    GIT_REPOSITORY https://github.com/trusch/libbcrypt.git
    GIT_TAG        master
)
FetchContent_MakeAvailable(libbcrypt)

# 5. Dotenv-CPP
FetchContent_Declare(
    dotenv-cpp
    GIT_REPOSITORY https://github.com/laserpants/dotenv-cpp.git
    GIT_TAG        master
)
FetchContent_MakeAvailable(dotenv-cpp)

# --- System Dependencies ---
find_package(OpenSSL REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Core Sql)
find_package(PostgreSQL REQUIRED) 

# Exiv2 via PkgConfig
find_package(PkgConfig REQUIRED)
pkg_check_modules(EXIV2 REQUIRED IMPORTED_TARGET exiv2)

# --- Sources & Headers ---
# Important: We use ${PROJECT_NAME} so the name is always correct
include_directories(include)
include_directories(${libbcrypt_SOURCE_DIR}/include)
include_directories(${ASIO_INCLUDE_DIR})
include_directories(${EXIV2_INCLUDE_DIRS}) # <--- Exiv2 Header

# Get all cpp files from src/ (Server + Worker Logic)
file(GLOB_RECURSE SOURCES "src/*.cpp")

# --- Executable ---
add_executable(${PROJECT_NAME} ${SOURCES})

# MOC for Qt
set_property(TARGET ${PROJECT_NAME} PROPERTY AUTOMOC ON)

# Linking
target_link_libraries(${PROJECT_NAME} PRIVATE
    Crow::Crow
    jwt-cpp::jwt-cpp
    bcrypt
    Qt6::Core
    Qt6::Sql
    Qt6::Gui
    Qt6::Concurrent
    PostgreSQL::PostgreSQL
    OpenSSL::SSL
    OpenSSL::Crypto
    PkgConfig::EXIV2
    dotenv
)


# --- INSTALLATION RULES (Für AppImage) ---

# 1. Das Executable nach /usr/bin installieren (Standard im AppImage)
install(TARGETS ${PROJECT_NAME} DESTINATION bin)

# 2. Den Static-Ordner (Frontend) installieren (falls du kein NGINX nutzt)
#    Falls du NGINX nutzt, brauchst du das hier nicht, aber für ein
#    Standalone-AppImage ist es sinnvoll.
if(EXISTS "${CMAKE_SOURCE_DIR}/static")
    install(DIRECTORY "${CMAKE_SOURCE_DIR}/static" DESTINATION bin)
endif()

# 3. Icon und Desktop-File (benötigt AppImage)
#    Wir erstellen Dummy-Dateien, falls sie nicht da sind, oder kopieren sie.
#    (Siehe Schritt 2 für die Erstellung der Dateien)
install(FILES "${CMAKE_SOURCE_DIR}/crowqt.desktop" DESTINATION share/applications)
install(FILES "${CMAKE_SOURCE_DIR}/crowqt.png"     DESTINATION share/icons/hicolor/256x256/apps)



################
message(STATUS "Build setup complete for ${PROJECT_NAME}")


