#ifndef GOLIATH_H
#define GOLIATH_H
#include <stdint.h>

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

#define Kilobytes(Value) ((Value) * 1024LL)
#define Megabytes(Value) (Kilobytes(Value) * 1024LL)
#define Gigabytes(Value) (Megabytes(Value) * 1024LL)
#define Terabytes(Value) (Gigabytes(Value) * 1024LL)

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

  real32 EndX;
  real32 EndY;

  real32 StartX;
  real32 StartY;

  real32 MinX;
  real32 MinY;

  real32 MaxX;
  real32 MaxY;

  union {
    game_button_state Buttons[6];
    struct {
      game_button_state Up;
      game_button_state Down;
      game_button_state Left;
      game_button_state Right;
      game_button_state LeftShoulder;
      game_button_state RightShoulder;
    };
  };
};

struct game_input {
  game_controller_input Controllers[4];
};

struct game_memory {
  bool32 IsInitialized;
  uint64 PermanentStorageSize;
  void *PermanentStorage;
  uint64 TransientStorageSize;
  void *TransientStorage;
};

void GameUpdateAndRender(game_memory *GameMemory, game_input *Input,
                         game_offscreen_buffer *Buffer,
                         game_sound_output_buffer *SoundBuffer);

struct game_state {
  int ToneHZ;
  int GreenOffset;
  int BlueOffset;
};
#endif // GOLIATH_H
