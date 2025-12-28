# Goliath Game Engine - Technical Context

This file provides technical context for working on the Goliath game engine codebase.

## Project Overview

Goliath is a handmade game engine following platform abstraction principles. The codebase demonstrates separation between platform-specific code and platform-independent game logic.

## Architecture

### Three-Tier Design

```
┌─────────────────────────────────────┐
│  Platform Layer (SDL/macOS)         │
│  - Event handling                   │
│  - Window/rendering setup           │
│  - Audio callbacks                  │
│  - Controller enumeration           │
└────────────┬────────────────────────┘
             │
             ▼
┌─────────────────────────────────────┐
│  Platform-Independent API           │
│  - game_input                       │
│  - game_memory                      │
│  - game_offscreen_buffer            │
│  - game_sound_output_buffer         │
└────────────┬────────────────────────┘
             │
             ▼
┌─────────────────────────────────────┐
│  Game Logic (goliath.cpp)           │
│  - GameUpdateAndRender()            │
│  - RenderWeirdGradient()            │
│  - GameOutputSound()                │
└─────────────────────────────────────┘
```

## Key Files

### Platform-Independent Code

**src/goliath.h**
- Defines all platform-independent data structures
- Core types: `game_input`, `game_memory`, `game_state`, `game_controller_input`
- Platform services API: `DEBUGPlatformReadEntireFile()`, `DEBUGPlatformWriteEntireFile()`
- Memory macros: `Kilobytes()`, `Megabytes()`, `Gigabytes()`

**src/goliath.cpp**
- Main game logic entry point: `GameUpdateAndRender()`
- Graphics: `RenderWeirdGradient()` - software rendering demo
- Audio: `GameOutputSound()` - sine wave tone generator
- Game state: Color offsets controlled by input

### Platform Layers

**src/sdl_goliath.cpp** (SDL2 cross-platform)
- Window and renderer setup with VSync
- Keyboard event handling → `KeyboardState` global
- Game controller polling (SDL_GameController API)
- Audio ring buffer and callback (`SDLAudioCallback()`)
- Input double-buffering with pointer swapping
- Keyboard mapped to Controllers[0], physical controllers to Controllers[1-5]

**src/macos/osx_main.mm** (macOS native)
- Cocoa/AppKit window and event loop
- Core Audio output queue
- IOKit HID for game controllers
- Keyboard also mapped as virtual controller

## Important Patterns

### 1. Input Double-Buffering

```cpp
game_input Input[2] = {};
game_input *NewInput = &Input[0];
game_input *OldInput = &Input[1];

// Every frame:
// 1. Poll input → write to NewInput
// 2. Call GameUpdateAndRender(NewInput)
// 3. Swap pointers
game_input *Temp = NewInput;
NewInput = OldInput;
OldInput = Temp;
```

**Why:** Allows game logic to see previous frame's state for edge detection (button pressed/released).

### 2. Audio Ring Buffer

```cpp
struct sdl_audio_ring_buffer {
  int Size;
  int WriteCursor;  // Where game writes new samples
  int PlayCursor;   // Where audio callback reads from
  void *Data;       // Circular buffer
};
```

**Flow:**
1. Audio callback runs in background thread, reads from `PlayCursor`
2. Main loop calculates how much to write based on latency
3. Game generates samples via `GameOutputSound()`
4. Platform copies samples to ring buffer at `WriteCursor`
5. Cursors chase each other around the circle

### 3. Virtual Controller Pattern

Keyboard input is mapped to `Controllers[0]` as if it were a game controller:
- WASD → MoveUp/Down/Left/Right
- Arrows → ActionUp/Down/Left/Right
- Q/E → LeftShoulder/RightShoulder
- Space/ESC → Start/Back
- `IsAnalog = false` (keyboard is digital)
- `IsConnected = true` (always available)

Physical controllers use indices 1-5.

### 4. Platform Services (Dependency Inversion)

Game code calls platform services via function pointers:

```cpp
// Defined by platform, called by game
debug_read_file_result DEBUGPlatformReadEntireFile(char *Filename);
void DEBUGPlatformFreeFileMemory(void *Memory);
bool32 DEBUGPlatformWriteEntireFile(char *Filename, uint32 MemorySize, void *Memory);
```

This keeps game code portable - platform layer provides implementations.

## Data Structures

### game_controller_input
```cpp
struct game_controller_input {
  bool32 IsAnalog;              // True if using analog stick
  bool32 IsConnected;           // Controller is attached
  real32 StickAverageX;         // Analog stick X (-1.0 to 1.0)
  real32 StickAverageY;         // Analog stick Y (-1.0 to 1.0)

  union {
    game_button_state Buttons[13];
    struct {
      game_button_state MoveUp/Down/Left/Right;
      game_button_state ActionUp/Down/Left/Right;
      game_button_state LeftShoulder/RightShoulder;
      game_button_state Back/Start;
      game_button_state Terminator;
    };
  };
};
```

### game_button_state
```cpp
struct game_button_state {
  int HalfTransitionCount;  // Number of times state changed this frame
  bool32 EndedDown;         // Current state (pressed = true)
};
```

**Usage:** Check `EndedDown` for current state, `HalfTransitionCount` for edge detection.

### game_memory
```cpp
struct game_memory {
  bool32 IsInitialized;
  uint64 PermanentStorageSize;  // Long-lived game state
  void *PermanentStorage;
  uint64 TransientStorageSize;  // Temporary/per-level data
  void *TransientStorage;
};
```

**Typical allocation:**
- Permanent: 64MB
- Transient: 4GB

## Build System

### SDL Build (build.sh)
```bash
clang++ \
  -DGOLIATH_INTERNAL=1 \
  -DGOLIATH_SLOW=1 \
  $(sdl2-config --cflags) \
  src/goliath.cpp \
  src/sdl_goliath.cpp \
  $(sdl2-config --libs) \
  -o build/goliath
```

Flags:
- `GOLIATH_INTERNAL`: Enables debug file I/O and asserts
- `GOLIATH_SLOW`: Enables runtime checks and validation

### macOS Build (src/macos/build_osx.sh)
Creates `macOS.app` bundle with:
- Main executable
- Info.plist
- Icon resources
- Linked against: Cocoa, IOKit, CoreAudio, AudioToolbox

## Coding Conventions

- `internal_usage` = `static` (internal linkage)
- `local_persist` = `static` (persistent local variable)
- `global_variable` = `static` (global variable)
- Type aliases: `int32`, `uint32`, `real32`, `real64`, `bool32`
- `Assert()` macro: Enabled in `GOLIATH_SLOW` mode
- `ArrayCount(arr)` macro: Returns array element count

## Common Workflows

### Adding New Input Buttons
1. Add to `game_controller_input` union in `goliath.h`
2. Expand `Buttons[]` array size
3. Map in platform layer (SDL/macOS event handlers)
4. Update keyboard mapping if needed
5. Use in `GameUpdateAndRender()`

### Adding New Platform Layer
1. Implement window/rendering setup
2. Implement input polling (keyboard + controllers)
3. Implement audio output (ring buffer pattern)
4. Implement debug file I/O functions
5. Allocate `game_memory` (mmap/VirtualAlloc)
6. Call `GameUpdateAndRender()` each frame

### Debugging
- Build with `-DGOLIATH_SLOW=1` for asserts
- Check `printf()` output for debug messages
- Use `DEBUGPlatformWriteEntireFile()` to dump state
- Monitor frame timing (MS/frame, FPS, megacycles/frame)

## Recent Changes

Last commit: `53f7064` - "feat(input): add keyboard support and refactor input system"
- Added keyboard as virtual controller (Controllers[0])
- Refactored button naming (Move*/Action* pattern)
- Added IsConnected flag
- Expanded controller array to 6 slots
- Added GetController() helper function
- Implemented analog-to-digital threshold conversion
- Added D-pad to analog stick mapping

## Current Limitations

- No hot-plug support (controllers must be connected at startup)
- Single-threaded (except audio callback)
- No asset loading system
- No entity/component system
- Simple test graphics (gradient only)
- Simple test audio (sine wave only)

## Future Considerations

When extending this codebase:
- Maintain platform abstraction separation
- Keep game code portable (no platform-specific APIs)
- Use platform services pattern for OS features
- Consider memory arena allocators for game state
- Plan for hot-reloading game DLL (handmade pattern)
