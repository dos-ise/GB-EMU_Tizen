# GB-EMU for Samsung Tizen TVs 🎮

A WebAssembly-powered port of **GB-EMU** — a Game Boy (DMG) emulator written from scratch in **C++17** — packaged as a Samsung Tizen TV application.
This project uses [GB-EMU](https://github.com/dos-ise/GB-EMU_Tizen) compiled to WebAssembly via Emscripten and wraps it into a Tizen widget (`.wgt`) that runs directly on Samsung Smart TVs.

> ⚠️ **WORK IN PROGRESS**: GB-EMU itself is under active development (incomplete CPU instruction set, MBC support, etc.) and intended for **educational and testing purposes only**.

---

## Features

- Runs entirely on the TV (no streaming required)
- **Loads ROMs straight from a USB stick** — no rebuild needed to play a different game, with an on-screen picker if multiple ROMs are found (falls back to a bundled example ROM if none are present)
- **Download ROMs over Wi-Fi** — fetch ROMs from a tiny HTTP server on your PC and store them on the TV; they keep working offline afterwards, no USB stick needed
- **In-game menu** (Blue button / Select+Start) to switch ROMs, download ROMs, or exit the app without restarting
- Supports Samsung TV remote control mapping (D-Pad, OK, Back, colored buttons)
- Supports standard gamepads/controllers (polled via the Gamepad API, D-Pad + left stick)
- Optimized for Samsung Tizen TV devices
- Simple installation using the Samsung Jellyfin Installer

---

## Screenshots

| ROM Picker (USB) | Gameplay | In-Game Menu |
|:---:|:---:|:---:|
| ![ROM select screen showing mario.gb and tetris.gb found on USB](docs/screenshots/rom-select.jpg) | ![Tetris title screen running on the TV](docs/screenshots/gameplay-tetris.jpg) | ![In-game menu with Resume, Change ROM and Exit App options](docs/screenshots/game-menu.jpg) |

---

## Installation

### 1. Enable Developer Mode on your TV

1. From the Apps screen, press `1 2 3 4 5` on the remote
2. Toggle **Developer Mode = ON**
3. Enter your PC's LAN IP in **Host PC IP**
4. Reboot the TV

### 2. Install the `.wgt`

#### Install with Apps2Samsung
The easiest way to install the generated `.wgt` file on your Samsung TV is by using Apps2Samsung:
Download the latest version from [Apps2Samsung](https://github.com/Apps2Samsung/Apps2Samsung/releases/latest), choose Tizen Community as release and choose GBEmu.
Launch GBEmu from the TV's app menu.

---

## Building

### Requirements
- Docker
- Git

### Quick Build

Run the build script to compile and extract the `.wgt` file:

**Windows:**
```batch
build.bat
```

This will:
1. Build the Docker image (Emscripten toolchain + GB-EMU compiled to WebAssembly + Tizen Studio CLI)
2. Assemble the Tizen widget (`config.xml`, `icon.png` from `res/`, compiled `gb-emu.html/.js/.wasm`, bundled `roms/examples.gb`)
3. Sign and package it as a `.wgt`
4. Extract `GBEmu.wgt` to the current directory

### Manual Build

If you prefer to build manually:

```bash
# Build the Docker image
docker build -t gbemu-tizen .

# Create and start a temporary container
docker create --name gbemu-tmp gbemu-tizen
docker start gbemu-tmp

# Extract the .wgt file
docker cp gbemu-tmp:/home/gbemu/GBEmu.wgt .

# Clean up
docker stop gbemu-tmp
docker rm gbemu-tmp
```

**macOS / Linux:**
```bash
docker build --platform linux/amd64 -t gbemu-tizen .
docker create --platform linux/amd64 --name gbemu-tmp gbemu-tizen
docker cp gbemu-tmp:/home/gbemu/GBEmu.wgt .
docker rm gbemu-tmp
```

> **Apple Silicon note:** the `--platform linux/amd64` flag is required on M-series Macs — Tizen Studio only ships x86-64 binaries, so a native `arm64` image fails during the Tizen Studio install (`rosetta error: failed to open elf`). Building as `amd64` runs the whole image under Rosetta instead. On Intel Macs and Linux the flag is harmless and can be dropped.

### Desktop build (browser testing)

The same page runs in a regular desktop browser — when it isn't running on Tizen it shows an **INSERT CARTRIDGE** button for loading a `.gb`/`.gbc` file, and the keyboard is mapped (arrows = D-pad, <kbd>Z</kbd>/<kbd>Enter</kbd> = A, <kbd>X</kbd> = B, <kbd>Shift</kbd> = SELECT/START, <kbd>Backspace</kbd> = game menu). With [Emscripten](https://emscripten.org/) installed locally:

```bash
emcmake cmake -DCMAKE_BUILD_TYPE=Release -S . -B build
cmake --build build

# Serve it (WASM won't load from file://)
python3 -m http.server -d build 8080
# then open http://localhost:8080/gb-emu.html
```

The `build/` directory is generated output and not tracked in git.

---

## Using Your Own ROMs

No rebuilding required — ROMs are loaded either from a **USB stick** plugged into the TV, or **downloaded over Wi-Fi** from your PC and stored on the TV, using the Tizen Filesystem API (`tizen.filesystem`).

### Option A: USB stick

1. Copy your **legally obtained** Game Boy / Game Boy Color ROM(s) (`.gb` / `.gbc`) to the root of a USB stick
2. Plug the stick into the TV and start the app
3. On startup, the app scans every mounted USB storage (plus any previously downloaded ROMs) for `.gb`/`.gbc` files:
   - **No ROM found** → falls back to the bundled example ROM (`roms/examples.gb`)
   - **Exactly one ROM found** → loads it automatically, no interaction needed
   - **More than one ROM found** → shows an on-screen picker (D-Pad/Stick to move, OK/A to load)

### Option B: Download over Wi-Fi (no USB)

The TV and your PC must be on the same network.

1. Put your ROMs in a folder on your PC and start the ROM server:
   ```bash
   ./tools/serve-roms.py path/to/roms      # defaults to ./roms on port 8000
   ```
   It prints the exact URL the app expects and serves a `roms.json` index automatically.
2. On the TV, open the in-game menu (**BLUE** button) and choose **Set ROM Server** to enter that address with the D-pad (LEFT/RIGHT = digit, UP/DOWN = change, OK = save). It's stored on the TV, so this is only needed once — or again whenever your PC's IP changes. (You can also bake your IP in as the default via `DEFAULT_ROM_SERVER_URL` in `src/index.html` before building.)
3. Back in the menu, choose **Download ROMs (Wi-Fi)**. Every ROM on the server is saved to the app's private storage (`wgt-private/roms/`) and shows up in the picker tagged `[saved]`.
4. Stop the server — downloaded ROMs keep working offline. Re-run the download any time to pick up new files (same-named files are overwritten).

**Note:** downloaded ROMs live in the app's private data folder, so they survive app restarts and updates, but are removed if you uninstall the app.

### Switching ROMs without restarting the app

Open the **in-game menu** at any time:

| Input | Action |
|-------|--------|
| **BACK** (remote) | Open/close game menu |
| **SELECT + START together** (gamepad) | Open/close game menu |

From the menu you can pick **Change ROM** to re-scan USB + saved ROMs and load a different one (the bundled example is always offered too), **Download ROMs (Wi-Fi)** to fetch new ROMs from your PC, **Set ROM Server** to point the app at your PC (stored on the TV), or **Exit App** to close GBEmu entirely — see [Controls](#controls) below.

**Note:** ROMs are only scanned at the top level of each USB drive (no subfolders). This repository does not include any copyrighted ROMs — you must own a legitimate copy of anything you copy to the stick or serve over Wi-Fi.

---

## Controls

### Samsung TV Remote
| Button | Action |
|--------|--------|
| **Arrow Keys** | D-Pad (movement) |
| **OK** | A |
| **Play/Pause** | B |
| **CH +** | SELECT |
| **CH −** | START |
| **BACK** | Open/close game menu (Resume / Change ROM / Download ROMs / Set ROM Server / Exit App) |

Only hard keys are used — the colored (RGYB) buttons are virtual on current Samsung remotes, so they're deliberately avoided.

**Limitation — remote B/SELECT/START are tap-only.** On Samsung remotes, every key except the D-pad and OK is delivered as an instantaneous click with no reliable key-release event, so the app synthesizes a short (~120 ms) button press per click. Tapping works perfectly, but these buttons **cannot be held down** — games that require holding B (e.g. holding B to run in platformers) need a Bluetooth gamepad, where all buttons support true press-and-hold.

### Controller
| Button | Action |
|--------|--------|
| **D-Pad / Left Stick** | Movement |
| **A** | A |
| **B** | B |
| **Select / Back** | SELECT |
| **Start** | START |
| **Select + Start** (held together) | Open/close game menu (Resume / Change ROM / Download ROMs / Set ROM Server / Exit App) |

---

## Project Structure

```
GB-EMU_Tizen/
├── core/                # Emulator logic (CPU, MMU, Cartridge, Mappers)
├── emc_main.cpp         # Main entry point for the Web version
├── CMakeLists.txt       # Build configuration
├── src/
│   └── index.html       # Game shell: TV remote + gamepad support, auto ROM load
├── res/
│   ├── config.xml       # Tizen widget manifest
│   └── icon.png         # App icon
├── roms/
│   └── examples.gb      # Bundled example ROM, loaded automatically on startup
├── tools/
│   └── serve-roms.py    # HTTP server for "Download ROMs (Wi-Fi)" on the TV
├── Dockerfile            # Build configuration
└── build.bat             # Build script (Windows)
```

---

## Project Status

* [x] ROM loading via Virtual File System
* [x] Basic emulator architecture
* [x] WebAssembly compilation pipeline
* [x] PPU (Graphics & Rendering)
* [x] Timers & Interrupts
* [x] Samsung TV remote control mapping
* [x] Gamepad/controller support
* [ ] Complete CPU instruction set — *In progress*
* [ ] Joypad input handling (core) — *In progress*
* [ ] MBC support — *In progress*

---

## Project Goals

This project aims to:

* Learn **Game Boy (DMG) architecture**
* Explore **low-level emulation concepts**
* Experiment with **C++ + WebAssembly**
* Build a full emulator **from scratch**, without external emulation libraries
* Bring homebrew/hobby emulation to Samsung Smart TVs

---

## Credits

- **GB-EMU** — https://github.com/dos-ise/GB-EMU_Tizen
- **Tizen Installer** - https://github.com/Jellyfin2Samsung/Samsung-Jellyfin-Installer
- **Emscripten** - https://emscripten.org/

---

## ⚖️ Legal Disclaimer

This project is an **independent, unofficial emulator** developed for **educational purposes only**.

- This repository does **NOT** include any copyrighted ROMs, BIOS files, or proprietary assets, aside from the bundled `examples.gb` test ROM.
- Users must provide their own legally obtained Game Boy ROMs if they wish to bundle a different game.
- This project is **not affiliated with, endorsed by, or associated with Nintendo**.

All trademarks and registered trademarks are the property of their respective owners.
