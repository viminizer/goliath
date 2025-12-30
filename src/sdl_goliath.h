#include "goliath.h"
#if !defined(SDL_HANDMADE_H)
#include <SDL.h>
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

struct sdl_offscreen_buffer {
  // NOTE(casey): Pixels are always 32-bits wide, Memory Order BB GG RR XX
  SDL_Texture *Texture;
  void *Memory;
  int Width;
  int Height;
  int Pitch;
  int BytesPerPixel;
};

struct sdl_window_dimension {
  int Width;
  int Height;
};

struct sdl_audio_ring_buffer {
  int Size;
  int WriteCursor;
  int PlayCursor;
  void *Data;
};

struct sdl_sound_output {
  int SamplesPerSecond;
  uint32 RunningSampleIndex;
  int BytesPerSample;
  int SecondaryBufferSize;
  int SafetyBytes;
};

struct sdl_debug_time_marker {
  int OutputPlayCursor;
  int OutputWriteCursor;
  int OutputLocation;
  int OutputByteCount;
  int ExpectedFlipPlayCursor;

  int FlipPlayCursor;
  int FlipWriteCursor;
};

struct sdl_game_code {
  void *GameCodeDLL;
  time_t DLLLastWriteTime;

  // Function pointers to game code
  game_update_and_render *UpdateAndRender;
  game_get_sound_samples *GetSoundSamples;

  bool32 IsValid;
};

// Input recording/playback
#define SDL_STATE_FILE_NAME_COUNT 4096

struct sdl_replay_buffer {
  int FileHandle;
  void *MemoryBlock;
  void *MemoryMap;
  char FileName[SDL_STATE_FILE_NAME_COUNT];
};

struct sdl_state {
  uint64 TotalSize;
  void *GameMemoryBlock;

  sdl_replay_buffer ReplayBuffers[4];

  int RecordingHandle;
  int InputRecordingIndex;

  int PlaybackHandle;
  int InputPlayingIndex;

  char EXEFileName[SDL_STATE_FILE_NAME_COUNT];
  char *OnePastLastEXEFileNameSlash;
};

#define SDL_HANDMADE_H
#endif
