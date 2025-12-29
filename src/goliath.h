#ifndef GOLIATH_H
#define GOLIATH_H
#include <stdint.h>
#define internal_usage static
#define local_persist static
#define global_variable static

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef float real32;
typedef double real64;
typedef bool bool32;

#if GOLIATH_SLOW
#define Assert(Expression)                                                     \
  do {                                                                         \
    if (!(Expression)) {                                                       \
      __builtin_trap();                                                        \
    }                                                                          \
  } while (0)
#else
#define Assert(Expression)
#endif

#define ArrayCount(Array) (sizeof(Array) / sizeof(Array[0]))

#if GOLIATH_INTERNAL
struct debug_read_file_result {
  void *Contents;
  uint32 ContentSize;
};
debug_read_file_result DEBUGPlatformReadEntireFile(char *Filename);
void DEBUGPlatformFreeFileMemory(void *Memory);
bool32 DEBUGPlatformWriteEntireFile(char *Filename, uint32 MemorySize,
                                    void *Memory);
#endif

#define Kilobytes(Value) ((Value) * 1024LL)
#define Megabytes(Value) (Kilobytes(Value) * 1024LL)
#define Gigabytes(Value) (Megabytes(Value) * 1024LL)
#define Terabytes(Value) (Gigabytes(Value) * 1024LL)

struct game_offscreen_buffer {
  void *Memory;
  int Width;
  int Height;
  int Pitch;
};

struct game_sound_output_buffer {
  int16 *Samples;
  int SamplesPerSecond;
  int SampleCount;
};

struct game_button_state {
  int HalfTransitionCount;
  bool32 EndedDown;
};

struct game_controller_input {

  bool32 IsAnalog;
  bool32 IsConnected;

  real32 StickAverageX;
  real32 StickAverageY;

  union {
    game_button_state Buttons[13];
    struct {
      game_button_state MoveUp;
      game_button_state MoveDown;
      game_button_state MoveLeft;
      game_button_state MoveRight;

      game_button_state ActionUp;
      game_button_state ActionDown;
      game_button_state ActionLeft;
      game_button_state ActionRight;

      game_button_state LeftShoulder;
      game_button_state RightShoulder;

      game_button_state Back;
      game_button_state Start;

      game_button_state Terminator;
    };
  };
};

struct game_keyboard_state {
  bool32 KEY_W;
  bool32 KEY_A;
  bool32 KEY_S;
  bool32 KEY_D;
  bool32 KEY_L; // For input recording/playback

  bool32 ARROW_UP;
  bool32 ARROW_DOWN;
  bool32 ARROW_LEFT;
  bool32 ARROW_RIGHT;

  bool32 LEFT_SHOULDER;
  bool32 RIGHT_SHOULDER;

  bool32 BACK;
  bool32 START;

  bool32 TERMINATOR;
};

struct game_input {
  // Mouse input
  int32 MouseX, MouseY, MouseZ;
  game_button_state MouseButtons[5];

  game_controller_input Controllers[6];

  // Hot reloading support
  bool32 ExecutableReloaded;
};

// NOTE(casey): Services that the platform layer provides to the game
struct platform_thread_context {
  int Placeholder;
};

struct game_memory {
  bool32 IsInitialized;
  uint64 PermanentStorageSize;
  void *PermanentStorage;
  uint64 TransientStorageSize;
  void *TransientStorage;
};

// NOTE: Function signature for game update and render
#define GAME_UPDATE_AND_RENDER(name) void name(platform_thread_context *ThreadContext, \
                         game_memory *GameMemory, game_input *Input, \
                         game_offscreen_buffer *Buffer, \
                         game_sound_output_buffer *SoundBuffer)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);

// NOTE: Function signature for getting sound samples
#define GAME_GET_SOUND_SAMPLES(name) void name(platform_thread_context *ThreadContext, \
                         game_memory *GameMemory, \
                         game_sound_output_buffer *SoundBuffer)
typedef GAME_GET_SOUND_SAMPLES(game_get_sound_samples);

#ifdef __cplusplus
extern "C" {
#endif

GAME_UPDATE_AND_RENDER(GameUpdateAndRender);

#ifdef __cplusplus
}
#endif

struct game_state {
  int ToneHZ;
  int GreenOffset;
  int BlueOffset;
};

#ifdef __cplusplus
extern "C" {
#endif

game_controller_input *GetController(game_input *Input, int ControllerIndex);

#ifdef __cplusplus
}
#endif

#endif // GOLIATH_H
