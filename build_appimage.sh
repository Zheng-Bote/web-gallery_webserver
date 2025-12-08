#!/bin/bash
set -e

# 1. Tools herunterladen (linuxdeploy & qt-plugin)
#    Wir prüfen, ob sie schon da sind, um Zeit zu sparen.
if [ ! -f linuxdeploy-x86_64.AppImage ]; then
    wget https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
    chmod +x linuxdeploy-x86_64.AppImage
fi

if [ ! -f linuxdeploy-plugin-qt-x86_64.AppImage ]; then
    wget https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage
    chmod +x linuxdeploy-plugin-qt-x86_64.AppImage
fi

# 2. Build-Verzeichnis säubern und neu erstellen
rm -rf AppDir build_appimage
mkdir build_appimage
cd build_appimage

# 3. CMake Konfigurieren & Bauen
#    WICHTIG: Wir installieren in ein lokales Verzeichnis "AppDir"
cmake .. -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
make install DESTDIR=../AppDir

cd ..

# 4. AppImage erstellen
#    Wir setzen Environment-Variablen für das Qt-Plugin
export QMAKE=$(which qmake6 || which qmake) # Pfad zu deinem qmake6 finden

echo "Erstelle AppImage..."

./linuxdeploy-x86_64.AppImage \
    --appdir AppDir \
    --plugin qt \
    --output appimage \
    --icon-file crowqt.png \
    --desktop-file crowqt.desktop

echo "Fertig! Das AppImage liegt im aktuellen Ordner."
