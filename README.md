# Roblox Game Maintenance Tool (RGMT)

RGMT is a lightweight Windows desktop application for managing and launching your favorite Roblox games. It provides a quick-launch library, retrieves game information from Roblox, and uses Bloxstrap to start games through the Roblox protocol.

> **Status:** Early development

## Features

- Add Roblox games using a game URL or numeric ID.
- Resolve Roblox Place IDs and Universe IDs automatically.
- Retrieve game names, creator information, and thumbnails through the Roblox web APIs.
- Display saved games as quick-launch cards.
- Launch games with the `roblox://` protocol.
- Remove games from the quick-launch list.
- Detect and install Bloxstrap through `winget` when necessary.
- Native Windows UI built with Qt 6.

## Requirements

- Windows 10 version 1809 or later.
- A working Roblox installation.
- [Qt 6](https://www.qt.io/) with the following components:
  - Qt Widgets
  - Qt Network
- CMake 3.20 or later.
- A C++20-compatible compiler, such as Visual Studio 2022.
- Windows App Installer / `winget` for automatic Bloxstrap installation.
- Internet access for Roblox API requests and thumbnail downloads.

## Building

1. Clone the repository:

   ```bash
   git clone https://github.com/neil289506-commits/RGMT.git
   cd RGMT
   ```

2. Configure the project with CMake:

   ```bash
   cmake -S . -B build
   ```

3. Build the application:

   ```bash
   cmake --build build --config Release
   ```

The resulting executable is named `RobloxGameMaintenanceTool.exe`.

### Qt configuration

If CMake cannot locate Qt 6, specify the Qt installation path manually. For example:

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:/Qt/6.x.x/msvc2022_64"
```

Replace the path with the Qt installation available on your system.

## Usage

1. Start `RobloxGameMaintenanceTool.exe`.
2. If Bloxstrap is not detected, allow RGMT to install it with `winget`, or install Bloxstrap manually.
3. Run Bloxstrap at least once so that the Roblox protocol handler is registered.
4. Select the **Add** button.
5. Paste a Roblox game URL or enter a numeric game ID.
6. Confirm the game information and add it to the quick-launch list.
7. Select a saved game to launch it.

RGMT stores saved game entries locally using the application settings system. Removing a game only removes it from RGMT's quick-launch list; it does not uninstall Roblox or delete the Roblox experience.

## Roblox API usage

RGMT communicates with Roblox's public web APIs to:

- Convert Place IDs to Universe IDs.
- Retrieve game metadata.
- Retrieve game thumbnails.

API availability, response formats, and rate limits are controlled by Roblox and may change over time.

## Project structure

- `src/MainWindow.*` — Main application window and quick-launch interface.
- `src/AddGameDialog.*` — Dialog for adding Roblox games.
- `src/GameEntry.*` — Persistent game-entry storage.
- `src/GameCardWidget.*` — Saved-game card UI.
- `src/RobloxApiClient.*` — Roblox API and thumbnail requests.
- `src/BloxstrapManager.*` — Bloxstrap detection and installation support.
- `src/SplashScreen.*` — Startup checks and initialization.
- `src/Theme.*` — Application styling.
- `CMakeLists.txt` — CMake build configuration.

## Limitations

- RGMT currently targets Windows desktop environments.
- Bloxstrap installation depends on `winget` and Windows App Installer.
- Launching games requires a correctly registered Roblox protocol handler.
- Network access is required when adding games and downloading metadata or thumbnails.

## License

RGMT is licensed under the GNU Lesser General Public License, version 2.1. See [`license.txt`](license.txt) for the full license text.
