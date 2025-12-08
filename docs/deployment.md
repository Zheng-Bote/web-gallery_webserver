# Deployment

**under construction**

Die Architektur im Container
Wir haben 3 Services (Container):

database: PostgreSQL (speichert Metadaten & User).

backend: Dein C++ CrowQtServer (verarbeitet API & Uploads).

frontend: NGINX (liefert Angular, Bilder und leitet API an Backend weiter).

Wichtig: Backend und Frontend teilen sich ein Volume (den Photos-Ordner), damit das Backend Bilder hineinschreiben und NGINX sie herauslesen kann.

Schritt 1: Ordnerstruktur vorbereiten
Damit Docker die Build-Kontexte sauber findet, empfehle ich folgende Struktur. Verschiebe deine Dateien entsprechend (oder passe die Pfade in den Dockerfiles an):

```
CrowGallery/
├── docker-compose.yml
├── backend/                 # C++ Projekt
│   ├── CMakeLists.txt
│   ├── src/
│   ├── include/
│   └── Dockerfile
└── frontend/                # Angular Projekt
    ├── angular.json
    ├── src/
    ├── package.json
    ├── Dockerfile
    └── nginx.conf
```

Schritt 2: Das Backend Dockerfile (backend/Dockerfile)
Wir nutzen einen Multi-Stage Build.

Builder: Ein Image mit allen Compilern (GCC, CMake) und Dev-Libraries. Baut die Exe.

Runtime: Ein schlankes Image, das nur die Laufzeit-Bibliotheken hat.

Datei: backend/Dockerfile

```Dockerfile
# --- STAGE 1: Builder ---
FROM ubuntu:24.04 AS builder

# Zeitzone und Updates (verhindert interaktive Abfragen)
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    pkg-config \
    libssl-dev \
    libpq-dev \
    qt6-base-dev \
    libqt6sql6-psql \
    libexiv2-dev \
    qt6-image-formats-plugins \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Wir kopieren erst nur CMakeLists (Caching!)
COPY CMakeLists.txt .
# Da wir FetchContent nutzen, kopieren wir den Rest erst später oder jetzt komplett
COPY include/ include/
COPY src/ src/

# Build Prozess
RUN mkdir build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    make -j$(nproc)

# --- STAGE 2: Runtime ---
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

# Nur Laufzeit-Bibliotheken installieren (viel kleineres Image)
RUN apt-get update && apt-get install -y \
    libqt6core6 \
    libqt6sql6 \
    libqt6sql6-psql \
    libqt6gui6 \
    libqt6network6 \
    libexiv2-27 \
    libssl3 \
    libpq5 \
    qt6-image-formats-plugins \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Das Executable aus dem Builder kopieren
COPY --from=builder /app/build/CrowQtServer .

# Environment für Qt (Headless)
ENV QT_QPA_PLATFORM=offscreen
# Default Ports
ENV PORT=8080

# Ordner für Bilder erstellen
RUN mkdir -p Photos

# Startbefehl
CMD ["./CrowQtServer"]
```

Schritt 3: Das Frontend Dockerfile (frontend/Dockerfile)
Auch hier Multi-Stage:

Node: Baut die Angular App.

NGINX: Liefert die fertigen HTML/JS Dateien aus.

Datei: frontend/Dockerfile

```Dockerfile
# --- STAGE 1: Build Angular ---
FROM node:20-alpine AS build

WORKDIR /app

# Dependencies installieren (Caching nutzen)
COPY package*.json ./
RUN npm ci

# Source kopieren und bauen
COPY . .
# Production Build
RUN npm run build -- --configuration production

# --- STAGE 2: Serve with NGINX ---
FROM nginx:alpine

# Eigene NGINX Config kopieren
COPY nginx.conf /etc/nginx/conf.d/default.conf

# Build-Output kopieren (Pfad anpassen, falls dein Projektname anders ist!)
# Check in angular.json: "outputPath": "dist/crow-gallery"
COPY --from=build /app/dist/crow-gallery/browser /usr/share/nginx/html

# Port freigeben
EXPOSE 80

CMD ["nginx", "-g", "daemon off;"]
```

Schritt 4: Die NGINX Konfiguration (frontend/nginx.conf)
Diese Datei landet im NGINX-Container. Wichtig ist hier der proxy_pass auf den Hostname backend (so heißt unser Service gleich in der docker-compose).

Datei: frontend/nginx.conf

```nginx
server {
    listen 80;
    server_name localhost;

    # Große Uploads erlauben
    client_max_body_size 100M;

    # Gzip Kompression (Performance!)
    gzip on;
    gzip_types text/plain application/javascript application/json text/css image/svg+xml;
    gzip_min_length 1024;

    # 1. Angular Frontend
    root /usr/share/nginx/html;
    index index.html;

    location / {
        try_files $uri $uri/ /index.html;
    }

    # 2. Bilder (Shared Volume)
    location /media/ {
        alias /var/www/photos/; # Interner Pfad im Container
        expires 30d;
        add_header Cache-Control "public";
    }

    # 3. API Proxy zum C++ Backend
    # Wichtig: "http://backend:8080" referenziert den Docker-Service-Namen!
    location ~ ^/(api|login|logout|upload) {
        proxy_pass http://backend:8080;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
    }
}
```

Schritt 5: Docker Compose (docker-compose.yml)
Hier fügen wir alles zusammen.

Datei: docker-compose.yml (im Hauptverzeichnis)

```yaml
services:
  # 1. Datenbank
  db:
    image: postgres:15-alpine
    restart: always
    environment:
      POSTGRES_USER: crow_user
      POSTGRES_PASSWORD: crow_password
      POSTGRES_DB: photos_db
    volumes:
      - db_data:/var/lib/postgresql/data

  # 2. C++ Backend
  backend:
    build: ./backend
    restart: always
    depends_on:
      - db
    environment:
      # Verbindung zur DB (Host ist der Service-Name "db")
      PG_HOST: db
      PG_PORT: 5432
      PG_USER: crow_user
      PG_PASS: crow_password
      PG_DB: photos_db
      # JWT Secret
      JWT_SECRET: mein_sehr_geheimes_docker_secret
      PORT: 8080
    volumes:
      # Wir teilen den Photos-Ordner mit dem Host und dem Frontend
      - ./photos_data:/app/Photos

  # 3. Frontend (NGINX + Angular)
  frontend:
    build: ./frontend
    restart: always
    ports:
      - "80:80" # Host-Port 80 auf Container-Port 80
    depends_on:
      - backend
    volumes:
      # NGINX muss die Bilder lesen können
      - ./photos_data:/var/www/photos:ro

volumes:
  db_data:
```

Docker starten: Gehe in das Hauptverzeichnis (wo die docker-compose.yml liegt) und führe aus:

```bash
docker compose up --build -d
```

- --build: Baut die Images neu (dauert beim ersten Mal ein paar Minuten wegen C++ und Angular Build).
- -d: Detached mode (läuft im Hintergrund).

Testen: Öffne `http://localhost` im Browser.

Was passiert:

Docker erstellt einen Ordner photos_data/ auf deinem Host-Rechner. Dort landen alle Bilder.

Docker erstellt ein Volume für die Datenbank (persistent).

Du hast ein voll isoliertes, produktives System.

Logs ansehen (falls was nicht geht):

```bash
docker compose logs -f backend
```
