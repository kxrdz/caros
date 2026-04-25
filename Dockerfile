# Dockerfile
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    # Build-Tools
    git cmake build-essential ninja-build \
    # OpenSceneGraph
    libopenscenegraph-dev \
    # SDL2
    libsdl2-dev libsdl2-mixer-dev \
    libsdl2-ttf-dev \
    # Netzwerk
    libenet-dev \
    libcurl4-gnutls-dev \
    # Grafik / Bilder
    libpng-dev libjpeg-dev \
    # Physik
    libplib-dev \
    # Audio
    libopenal-dev \
    libogg-dev libvorbis-dev \
    # Parser / Utils
    libexpat1-dev \
    libcjson-dev \
    librhash-dev \
    # Kompression
    zlib1g-dev libminizip-dev \
    # 3D Math
    libglm-dev \
    # Java
    openjdk-17-jdk openjdk-17-jre \
    && apt-get clean && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace

COPY bin/build.sh /build.sh
RUN chmod +x /build.sh

CMD ["/bin/bash"]