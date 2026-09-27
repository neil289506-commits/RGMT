#pragma once

#include <QString>

// Detects and installs Bloxstrap (https://github.com/pizzaboxer/bloxstrap)
// using several independent heuristics, since Bloxstrap does not always
// register itself the same way across versions/install modes.
//
// Sources verified against the Bloxstrap wiki (2026-09):
//  - "A deep dive on how the Roblox bootstrapper works" — confirms Roblox
//    registers the roblox-player protocol, and that Bloxstrap mimics this.
//  - "Bloxstrap doesn't launch with Roblox" — confirms the roblox-player
//    protocol handler is only (re-)registered the FIRST time Bloxstrap is
//    actually run (e.g. via its Start Menu / desktop shortcut), not simply
//    by installing it. This is why installViaWinget() launches Bloxstrap
//    once after a fresh install.
class BloxstrapManager
{
public:
    // Returns true if any detection method finds a Bloxstrap installation.
    static bool isInstalled();

    // Best-effort path to the installed Bloxstrap.exe, or empty if unknown.
    static QString installedExecutablePath();

    // Runs `winget install -e --id pizzaboxer.Bloxstrap` synchronously
    // (pumps the Qt event loop internally so the UI stays responsive).
    // On success, also launches Bloxstrap once so it registers itself as
    // the roblox-player protocol handler (see class comment above).
    // Returns true if the install (and handler registration) succeeded.
    static bool installViaWinget(QString *errorOut = nullptr);

private:
    static bool checkRegistryUninstallEntry();
    static bool checkLocalAppDataFolder();
    static bool checkProtocolHandler();
    static void registerAsProtocolHandler();
};
