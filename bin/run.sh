#!/bin/bash

echo "🔨 Baue..."
docker compose run --rm speed-dreams-build bash -c \
  "cd /workspace/build && ninja -j$(nproc) && ninja install"

if [ $? -eq 0 ]; then
  echo "Build erfolgreich, starte Spiel..."
  ALSOFT_CONF="$(pwd)/config/alsoft.conf" ./install-output/games/speed-dreams-2
else
  echo "Build fehlgeschlagen!"
fi
