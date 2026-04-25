# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Overview

This repo is a Docker-based build harness for **Speed Dreams** — an open-source motorsport simulator (fork of TORCS). The source lives in `speed-dreams-code/` (a git checkout), and Docker provides a reproducible Ubuntu 24.04 build environment with all dependencies pre-installed.

## Build & Run

**Full build inside Docker (first time or after CMake changes):**
```bash
docker compose run --rm speed-dreams-build bash /build.sh
```

**Incremental rebuild + launch game (uses cached build):**
```bash
./bin/run.sh
```

**Interactive Docker shell (for manual cmake/ninja invocations):**
```bash
docker compose run --rm speed-dreams-build bash
# Inside container:
cd /workspace/build && ninja -j$(nproc) && ninja install
```

**Build type:** Controlled via `BUILD_TYPE` env var (default: `Debug`). Set in `docker-compose.yml` or override on the command line:
```bash
BUILD_TYPE=Release docker compose run --rm speed-dreams-build bash /build.sh
```

## Directory Layout

| Path | Purpose |
|------|---------|
| `bin/build.sh` | CMake configure + Ninja build + install script (runs inside container) |
| `bin/run.sh` | Host-side script: triggers incremental Docker build then launches the game binary |
| `Dockerfile` | Ubuntu 24.04 image with all C++ dependencies |
| `docker-compose.yml` | Mounts `speed-dreams-code/` → `/workspace/source`, `build-output/` → `/workspace/build`, `install-output/` → `/workspace/install` |
| `speed-dreams-code/` | Upstream Speed Dreams source (CMakeLists.txt at root) |
| `build-output/` | CMake build artifacts (persisted via volume mount) |
| `install-output/` | Installed game files; binary at `install-output/games/speed-dreams-2` |

## Speed Dreams Source Architecture (`speed-dreams-code/src/`)

- **`libs/`** — Core shared libraries: `tgf` (game framework), `tgfclient` (UI/rendering client), `tgfdata` (data management), `robottools` (AI robot utilities), `math`, `portability`, `txml`, `learning`, `ephemeris`
- **`modules/`** — Runtime-loaded plugins: `simu` (physics simulation), `graphic` (rendering via OpenSceneGraph), `racing` (race logic), `sound` (OpenAL), `track` (track loading), `networking`/`csnetworking`, `userinterface`, `telemetry`
- **`drivers/`** — AI robot drivers (`simplix`, `shadow`, `dandroid`, `usr`, `urbanski`) plus `human` (player input) and `replay`
- **`main/`** — Game entry point
- **`interfaces/`** — Shared C headers defining module interfaces (the plugin contract between core and modules)
- **`tools/`** — Build-time utilities

The architecture is plugin-based: modules are shared libraries loaded at runtime. `interfaces/` defines the ABI contracts. Physics (`simu`), graphics, and AI drivers are all swappable modules.

## CMake Options

Key flag used in `build.sh`:
- `-DOPTION_OFFICIAL_ONLY=ON` — builds only officially supported content (reduces build time)

The game data (tracks, cars, textures) lives in `speed-dreams-code/speed-dreams-data/` as a git submodule — ensure it's initialized before building:
```bash
cd speed-dreams-code && git submodule update --init --recursive
```
