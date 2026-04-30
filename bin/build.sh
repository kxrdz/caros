#!/bin/bash
set -e

SOURCE_DIR=/workspace/source
BUILD_DIR=/workspace/build
INSTALL_DIR=/workspace/install

mkdir -p "$BUILD_DIR" "$INSTALL_DIR"

cd "$BUILD_DIR"

cmake "$SOURCE_DIR" \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=${BUILD_TYPE:-Debug} \
    -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR" \
    -DOPTION_OFFICIAL_ONLY=ON

ninja -j$(nproc)
ninja install

echo "Build fertig!"