# TinyTroopers

Top-down multiplayer shooter prototype in C++ with a Raylib client and UDP client-server backend.

---

## Prerequisites

### Windows
- **CMake** (3.20+)
- **C++20 Compiler**: Visual Studio 2019/2022 (MSVC) or MinGW-w64
- **Raylib**:
  - Official installer to `C:\raylib` (w64devkit), or
  - Installed via [vcpkg](https://vcpkg.io/) / package manager and added to `CMAKE_PREFIX_PATH`.

### macOS
- **Homebrew** installed
- Install CMake and Raylib:
  ```bash
  brew install cmake raylib
  ```

### Linux
- **C++20 Compiler** (`g++` 10+ or `clang++` 10+)
- **CMake** (3.20+)
- **Raylib**:
  - **Ubuntu / Debian**:
    ```bash
    sudo apt update
    sudo apt install -y cmake g++ libraylib-dev
    ```
  - **Arch Linux**:
    ```bash
    sudo pacman -S cmake gcc raylib
    ```
  - **Fedora**:
    ```bash
    sudo dnf install -y cmake gcc-c++ raylib-devel
    ```

---

## Build

### Windows (PowerShell / Command Prompt)

```powershell
cmake -S . -B build
cmake --build build --config Release
```

### macOS / Linux (Terminal)

```bash
# On macOS, pass CMAKE_PREFIX_PATH so CMake locates Homebrew's raylib
cmake -S . -B build -DCMAKE_PREFIX_PATH="$(brew --prefix 2>/dev/null || echo /usr)"
cmake --build build
```

---

## Run

> [!NOTE]
> Always launch the server before starting client instances.

### Windows

```powershell
# Start the server:
.\build\Release\TinyTroopersServer.exe
# (or .\build\TinyTroopersServer.exe if using MinGW/Ninja)

# Start a client (run one per player):
.\build\Release\TinyTroopersClient.exe
# (or .\build\TinyTroopersClient.exe if using MinGW/Ninja)
```

### macOS / Linux

```bash
# In your first terminal, start the server:
./build/TinyTroopersServer

# In separate terminal windows, start each client:
./build/TinyTroopersClient
```

---

## How to Play

- **Default Address**: The client connects to `127.0.0.1:42069` by default.
- **Joining / Hosting**: Use the in-game Join screen to specify a different host IP if connecting across a local network or server.
- **Host Role**: The first connected player becomes the room host and can start the match once players have joined.
- **Game Modes**: Team Deathmatch (TDM) requires an even player count before the host can start the match.

