# GB-EMU for Samsung Tizen TVs 🎮

A WebAssembly-powered port of **GB-EMU** — a Game Boy (DMG) emulator written from scratch in **C++17** — packaged as a Samsung Tizen TV application.
This project uses [GB-EMU](https://github.com/dos-ise/GB-EMU_Tizen) compiled to WebAssembly via Emscripten and wraps it into a Tizen widget (`.wgt`) that runs directly on Samsung Smart TVs.

> ⚠️ **WORK IN PROGRESS**: GB-EMU itself is under active development (incomplete CPU instruction set, MBC support, etc.) and intended for **educational and testing purposes only**.

---

## Features

- Runs entirely on the TV (no streaming required)
- **Loads ROMs straight from a USB stick** — no rebuild needed to play a different game, with an on-screen picker if multiple ROMs are found (falls back to a bundled example ROM if none are present)
- **In-game menu** (Blue button / Select+Start) to switch ROMs or exit the app without restarting
- Supports Samsung TV remote control mapping (D-Pad, OK, Back, colored buttons)
- Supports standard gamepads/controllers (polled via the Gamepad API, D-Pad + left stick)
- Optimized for Samsung Tizen TV devices
- Simple installation using the Samsung Jellyfin Installer

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

---

## Using Your Own ROMs

No rebuilding required — ROMs are loaded from a **USB stick** plugged into the TV, using the Tizen Filesystem API (`tizen.filesystem`).

### How it works

1. Copy your **legally obtained** Game Boy (`.gb`) to the root of a USB stick
2. Plug the stick into the TV and start the app
3. On startup, the app scans every mounted USB storage for `.gb` files:
   - **No ROM found** → falls back to the bundled example ROM (`roms/examples.gb`)
   - **Exactly one ROM found** → loads it automatically, no interaction needed
   - **More than one ROM found** → shows an on-screen picker (D-Pad/Stick to move, OK/A to load)

### Switching ROMs without restarting the app

Open the **in-game menu** at any time:

| Input | Action |
|-------|--------|
| **BLUE** (remote) | Open/close game menu |
| **SELECT + START together** (gamepad) | Open/close game menu |

From the menu you can pick **Change ROM (USB)** to re-scan the stick and load a different ROM (the bundled example is always offered too), or **Exit App** to close GBEmu entirely — see [Controls](#controls) below.

**Note:** ROMs are only scanned at the top level of each USB drive (no subfolders). This repository does not include any copyrighted ROMs — you must own a legitimate copy of anything you copy to the stick.

---

## Controls

### Samsung TV Remote
| Button | Action |
|--------|--------|
| **Arrow Keys** | D-Pad (movement) |
| **OK** | START |
| **RED** | A |
| **YELLOW** | SELECT |
| **BACK** | B |
| **BLUE** | Open/close game menu (Resume / Change ROM / Exit App) |

### Controller
| Button | Action |
|--------|--------|
| **D-Pad / Left Stick** | Movement |
| **A** | A |
| **B** | B |
| **Select / Back** | SELECT |
| **Start** | START |
| **Select + Start** (held together) | Open/close game menu (Resume / Change ROM / Exit App) |

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
