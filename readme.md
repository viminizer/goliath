# Goliath Game Engine

![Goliath Game Engine](data/logos/goliath-minimal.png)

A handmade game engine built from scratch following platform abstraction principles. This is a learning project focused on understanding low-level game engine architecture without relying on large frameworks.

## About

Goliath is a minimal game engine implementation that demonstrates:
- Platform abstraction layer design
- Cross-platform input handling (keyboard + game controllers)
- Audio output using ring buffers
- Software rendering to offscreen buffers
- Memory management and allocation strategies

The project currently implements two platform layers:
- **macOS native** (Objective-C++ with Cocoa/AppKit)
- **SDL2** (cross-platform)

Both share the same platform-independent game code.

## Current Features

- **Platform-Independent Game Layer**: Core game logic separated from platform code
- **Input System**: Unified keyboard and game controller input abstraction
  - Keyboard mapped to controller (Controllers[0])
  - Physical controllers mapped to Controllers[1-5]
  - Double-buffering pattern for input state
- **Audio System**: Ring buffer-based audio output with sine wave generation
- **Graphics**: Simple software rendering (animated gradient demo)
- **Memory**: Custom memory allocation with permanent and transient storage
- **Debug File I/O**: Platform-independent file read/write utilities

## Project Structure

```
goliath/
├── src/
│   ├── goliath.cpp         # Platform-independent game logic
│   ├── goliath.h           # Game API and data structures
│   ├── sdl_goliath.cpp     # SDL2 platform layer
│   ├── sdl_goliath.h       # SDL platform definitions
│   └── macos/
│       ├── osx_main.mm     # macOS native platform layer
│       └── osx_main.h      # macOS platform definitions
├── build.sh                # SDL build script
└── run.sh                  # Run SDL build
```

## Getting Started

### Prerequisites

**For SDL2 build:**
- SDL2 library (`brew install sdl2` on macOS)
- C++ compiler (clang/gcc)

**For macOS native build:**
- macOS with Xcode Command Line Tools
- Objective-C++ compiler

### Building and Running

**SDL2 version (recommended for cross-platform):**

```bash
./build.sh          # Builds to build/goliath
./run.sh            # Builds and runs
```

**macOS native version:**

```bash
cd src/macos
./build_osx.sh      # Builds macOS.app bundle
./run_osx.sh        # Builds and runs
```

### Controls

- **WASD**: Movement (mapped to controller D-pad)
- **Arrow Keys**: Action buttons
- **Q/E**: Shoulder buttons
- **Space**: Start button
- **ESC**: Back button
- **Alt+F4**: Quit (SDL)

Game controllers are automatically detected and supported.

## Architecture

The engine uses a clean platform abstraction architecture:

1. **Platform Layer** (SDL/macOS): Handles windowing, input events, audio callbacks
2. **Platform-Independent Layer** (goliath.cpp): Game logic, rendering, sound generation
3. **Data Flow**: Platform → Game → Platform (via function pointers and structs)

Key patterns:
- Double-buffering for input state
- Ring buffer for audio streaming
- Callback-based debug I/O
- Union types for flexible data access

## Current State

This is an **educational project** focused on learning engine architecture. Current capabilities:
- Displays animated gradient (controlled by keyboard/gamepad)
- Plays sine wave audio tone
- Demonstrates platform abstraction patterns
- Shows input handling architecture

Future plans may include: sprite rendering, tile maps, entity systems, and game-specific logic.

## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details.
