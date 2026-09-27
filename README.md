# Roblox Game Maintenance Tool (RGMT)

A Qt 6 / C++ desktop app for Windows that keeps [Bloxstrap](https://github.com/pizzaboxer/bloxstrap) installed and maintains a personal quick-launch grid of Roblox experiences — optionally synced to a private GitHub repository.

> Built with Qt **6.11.0**, MSVC **2022**.

---

## Table of Contents

- [Features](#features)
- [Install via winget](#install-via-winget)
- [Build from source](#build-from-source)
- [How it works](#how-it-works)
  - [Bloxstrap detection & install](#bloxstrap-detection--install)
  - [Adding a game](#adding-a-game)
  - [Remote sync](#remote-sync)
  - [Token security design](#token-security-design)
- [Project layout](#project-layout)
- [Sources I verified](#sources-i-verified)
- [Things I could not verify — please test these first](#things-i-could-not-verify--please-test-these-first)
- [License](#license)

---

## Features

- 🚦 **Splash-screen Bloxstrap check** — shows "Checking Bloxstrap" for at least 1 second, detects an existing install via three independent methods, and silently runs `winget install -e --id pizzaboxer.Bloxstrap` if it's missing.
- 🗂️ **Quick-launch grid** — a 4×3 card grid by default (more games just scroll with the mouse wheel), each card showing the game's thumbnail, name, and creator.
- ➕ **Add by link or ID** — paste a Roblox game link or a bare PlaceID/UniverseID; the app resolves it, downloads the name/creator/thumbnail from the public Roblox API, and saves it locally.
- ▶️ **One-click launch** — launches straight into the experience via the `roblox://` protocol (through Bloxstrap), or opens Bloxstrap itself with one click.
- 🗑️ **Safe delete** — always asks for confirmation before removing a quick-launch entry.
- ☁️ **Optional GitHub-backed remote sync**:
  - First run with no remote configured lets you **initialize a new private repo** or **link an existing one**.
  - Every add/delete **force-pushes to `master`**, so your quick-launch list follows you across machines.
  - **Pull** on every app start, plus a manual **Pull** button in the header.
  - GitHub token is checked (and re-prompted if expired) on every launch, and can be swapped at any time via the **Token** button.
- 🎨 Light/dark theme that follows the OS, with a small drawn "+" FAB button and card hover shadows.

## Install via winget

```powershell
winget install --id Neil.RGMT -e
```

> **Note:** `Neil.RGMT` is the package ID this project is meant to be published under, but publishing to the community `winget-pkgs` repository is a separate step (a signed release + a PR to [microsoft/winget-pkgs](https://github.com/microsoft/winget-pkgs)) that has to be done by the maintainer after cutting a release. A starter manifest is included under [`winget-manifest/`](winget-manifest/) — see [Sources I verified](#sources-i-verified) for the submission process and [Things I could not verify](#things-i-could-not-verify--please-test-these-first) for what's still a placeholder in it.

## Build from source

1. Install [Git for Windows](https://git-scm.com/download/win) (required for remote sync) and make sure `git` is on `PATH`.
2. Open `CMakeLists.txt` in Qt Creator.
3. Select the **Desktop Qt 6.11.0 MSVC2022 64-bit** kit.
4. Required Qt modules: `Widgets`, `Network` (already declared via `find_package` in `CMakeLists.txt`).
5. Build → Run.

Command-line alternative (from a VS 2022 Developer shell):

```powershell
cmake -B build -G "Visual Studio 17 2022" -DCMAKE_PREFIX_PATH="C:\Qt\6.11.0\msvc2022_64"
cmake --build build --config Release
```

## How it works

### Bloxstrap detection & install

`BloxstrapManager` ORs together three checks:

- `HKCU\...\Uninstall\Bloxstrap` (the uninstall entry Bloxstrap's Velopack installer registers)
- `%LocalAppData%\Bloxstrap\Bloxstrap.exe` existing on disk
- `HKCU\Software\Classes\roblox-player\shell\open\command` pointing at Bloxstrap

If none match, it runs:

```
winget install -e --id pizzaboxer.Bloxstrap --accept-source-agreements --accept-package-agreements
```

Per the Bloxstrap wiki, the `roblox-player` protocol handler is only registered the **first time Bloxstrap actually runs** — not just on install — so `installViaWinget()` launches the freshly-installed `Bloxstrap.exe` once afterwards to complete that registration.

### Adding a game

1. Paste a link or ID into the "+" dialog.
2. `RobloxApiClient` tries it as a **PlaceID** first (`GET /universes/v1/places/{id}/universe`); if that fails, it's treated as a **UniverseID** directly.
3. `GET /v1/games?universeIds={id}` supplies the name, creator, and canonical `rootPlaceId`.
4. `GET /v1/games/icons?universeIds={id}` supplies the thumbnail image.
5. Saved to `Game\<PlaceID>.ini` and `ThumbNail\<PlaceID>.png`, both relative to the executable's folder.

### Remote sync

`GameEntry` already stores everything under `Game\` and `ThumbNail\` inside the app's own folder — which is exactly the folder `GitRunner` treats as the local git repository. On startup, `MainWindow::initializeRemoteSync()`:

1. Loads and **validates the saved GitHub token** (`GET /user`); if it's missing or the API returns `401`, `TokenPromptDialog` asks for a new one (this also doubles as the "change token" flow, reachable any time via the header's **Token** button).
2. If the app folder isn't a git repo with an `origin` remote yet, shows `RemoteSetupDialog`, offering:
   - **Create a new private repository** — calls `POST /user/repos` (`private: true`), then `git init` / `git remote add` / initial commit / `git push -f origin HEAD:master`.
   - **Link an existing repository** — takes `owner/repo` or a full URL, sets it as `origin`, and pulls.
3. Runs `git pull origin master --allow-unrelated-histories`.

Every add/delete afterwards calls `syncPushChanges()`, which does `git add -A && git commit -m "..."` followed by **`git push -f origin HEAD:master`**, exactly as requested. A `.gitignore` is written automatically so only `Game/` and `ThumbNail/` are ever tracked — the app's own binaries never get committed.

> ⚠️ **Force-push caveat:** `git push -f` overwrites whatever is on `master`, including changes from another machine that hasn't pulled yet. This is what was asked for, but it means two machines editing the list without pulling in between can silently lose one side's changes. If that's a problem in practice, the fix is to make `syncPushChanges()` pull-then-push (or use a non-force push and resolve conflicts) instead of blindly forcing.

### Token security design

The GitHub token is never stored in plain text. `SecureTokenStore` layers three mechanisms, all via the Windows CryptoAPI (`wincrypt.h`):

1. **AES-256 (CBC)** encrypts the token bytes themselves, with a random IV.
2. **RSA-4096** — a key pair generated once and kept inside a private, per-user CryptoAPI key container — wraps (encrypts) the AES-256 session key. This is Microsoft's documented "envelope encryption" pattern (`CryptGenKey` + `CryptExportKey`/`CryptImportKey` with `SIMPLEBLOB`).
3. **DPAPI** (`CryptProtectData`/`CryptUnprotectData`) seals the entire package (IV + wrapped key + ciphertext) at rest, so the vault file on disk is only usable by this Windows account on this machine.

The vault lives at `%AppData%\RGMT\token.vault` — deliberately **outside** the git-tracked app folder, so it can never accidentally end up committed to the remote repo.

## Project layout

```
RobloxGameMaintenanceTool/
├── CMakeLists.txt
├── README.md
├── winget-manifest/
│   └── manifests/n/Neil/RGMT/1.0.0/   # starter winget manifest (see caveats)
└── src/
    ├── main.cpp
    ├── Theme.h / .cpp                 light/dark app-wide QSS
    ├── SplashScreen.h / .cpp          startup splash + Bloxstrap check/install
    ├── BloxstrapManager.h / .cpp      Bloxstrap detection + winget install
    ├── MainWindow.h / .cpp            main window, grid, header actions, remote sync flow
    ├── RoundButton.h / .cpp           hand-drawn circular "+" FAB button
    ├── GameCardWidget.h / .cpp        one quick-launch card (thumbnail, name, play/delete)
    ├── FlowLayout.h / .cpp            wrapping grid layout for the cards
    ├── AddGameDialog.h / .cpp         "+" dialog: resolve a link/ID into a GameEntry
    ├── RobloxApiClient.h / .cpp       Roblox public API calls
    ├── GameEntry.h / .cpp             Game\*.ini + ThumbNail\*.png persistence
    ├── SecureTokenStore.h / .cpp      AES-256 + RSA-4096 + DPAPI token vault
    ├── GitHubClient.h / .cpp          GitHub REST API (token check, repo creation)
    ├── GitRunner.h / .cpp             synchronous wrapper around the `git` CLI
    ├── TokenPromptDialog.h / .cpp     enter/validate/change the GitHub token
    └── RemoteSetupDialog.h / .cpp     first-run "init vs link" remote setup
```

## Sources I verified

Because this environment has no Windows/MSVC/Qt/git to actually run against, everything below was checked against documentation before writing the corresponding code, so you can audit it:

- **Bloxstrap detection heuristics & first-run protocol registration** — checked against the Bloxstrap wiki ("Bloxstrap doesn't launch with Roblox").
- **`roblox://` launch URI** (`roblox://experiences/start?placeId=<id>`) and the **`roblox-player:1+launchmode:app`** "just open the client" URI — both checked against the Bloxstrap wiki's "A deep dive on how the Roblox bootstrapper works", which gives these as the exact URIs the Roblox website itself uses.
- **winget-pkgs submission process** — checked against the `microsoft/winget-pkgs` repository's own README: manifests go under `manifests/<first-lowercase-letter-of-publisher>/<Publisher>/<Package>/<Version>/`, are validated locally with `winget validate`, and submitted as a PR (one manifest/installer per PR). `winget-create` can automate generating + submitting that PR.
- **CryptoAPI envelope-encryption call sequence** (`CryptGenKey` → `CryptExportKey`/`CryptImportKey` with `SIMPLEBLOB`, `CryptProtectData`/`CryptUnprotectData` for DPAPI) — written to match Microsoft's documented pattern for this exact use case (wrapping a symmetric key with an asymmetric one).

## Things I could not verify — please test these first

- **`SecureTokenStore` has not been compiled or run.** Win32 CryptoAPI code is easy to get subtly wrong (buffer sizing, blob formats, error codes). Build it first and send me any `CryptXxx failed: 0x...` codes you hit — I can fix them from the error code alone.
- **The three Roblox public API endpoints** (`universes/v1/places/{id}/universe`, `games/v1/games`, `games/v1/games/icons`) are implemented per their widely-documented public shapes, but I didn't send a real request to them from here — please try adding a real game once you can build and run this.
- **winget-pkgs currently documents accepted installer types as MSIX/MSI/APPX/.exe** in its top-level README, while the manifest schema itself also supports `zip`/`portable`. The included starter manifest uses `zip` + a `portable` nested installer, which is the easiest thing for a solo dev to produce without building a full installer — but if that gets rejected in review, switching to an actual installer (e.g. built with [Inno Setup](https://jrsoftware.org/isinfo.php)) and `InstallerType: exe` is the documented-safe fallback. Either way, `InstallerUrl` and `InstallerSha256` in `Neil.RGMT.installer.yaml` are placeholders you'll need to fill in from a real GitHub Release.
- **`winget install -e --id pizzaboxer.Bloxstrap`** — the package ID you specified; I didn't independently re-verify it's still current on the winget repository.

## License

Add a license of your choice here (e.g. MIT) — currently unspecified.
