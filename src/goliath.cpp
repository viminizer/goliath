#include "goliath.h"
// Ensure C linkage for dynamic library export
extern "C" {
#define M_PI 3.1415

internal_usage void GameOutputSound(game_state *GameState,
                                    game_sound_output_buffer *SoundBuffer,
                                    int ToneHZ) {
  local_persist real32 tSine;
  int16 ToneVolume = 3000;
  int WavePeriod = SoundBuffer->SamplesPerSecond / ToneHZ;
  int16 *SampleOut = SoundBuffer->Samples;

  for (int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount;
       ++SampleIndex) {
    int16 SampleValue = 0;
    *SampleOut++ = SampleValue;
    *SampleOut++ = SampleValue;
    tSine += 2.0f * M_PI * 1.0f / (real32)WavePeriod;
  }
}

internal_usage int32 RoundReal32ToInt32(real32 Value) {
  int32 Result = (int32)(Value + 0.5f);
  return Result;
}

internal_usage void DrawRectangle(game_offscreen_buffer *Buffer,
                                  real32 RealMinX, real32 RealMinY,
                                  real32 RealMaxX, real32 RealMaxY,
                                  uint32 Color) {

  uint32 MinX = RoundReal32ToInt32(RealMinX);
  uint32 MinY = RoundReal32ToInt32(RealMinY);
  uint32 MaxX = RoundReal32ToInt32(RealMaxX);
  uint32 MaxY = RoundReal32ToInt32(RealMaxY);

  if (MinX < 0) {
    MinX = 0;
  }
  if (MinY < 0) {
    MinY = 0;
  }
  if (MaxX > Buffer->Width) {
    MaxX = Buffer->Width;
  }
  if (MaxY > Buffer->Height) {
    MaxY = Buffer->Height;
  }
  uint8 *Row = ((uint8 *)Buffer->Memory + MinX * Buffer->BytesPerPixel +
                MinY * Buffer->Pitch);
  for (int Y = MinY; Y < MaxY; ++Y) {
    uint32 *Pixel = (uint32 *)Row;
    for (int X = MinX; X < MaxX; ++X) {
      *Pixel = Color;
    }
    Row += Buffer->Pitch;
  }
}

void GameUpdateAndRender(platform_thread_context *ThreadContext,
                         game_memory *GameMemory, game_input *Input,
                         game_offscreen_buffer *Buffer,
                         game_sound_output_buffer *SoundBuffer) {
  game_state *GameState = (game_state *)GameMemory->PermanentStorage;
  if (!GameMemory->IsInitialized) {
    GameMemory->IsInitialized = true;
  }

  DrawRectangle(Buffer, 0.0f, 0.0f, (real32)Buffer->Width,
                (real32)Buffer->Height, 0x0F00F0);

  DrawRectangle(Buffer, 100.0f, 100.0f, 5000.0f, 300.0f, 0xFFFFFF);

  game_controller_input *Controller = &Input->Controllers[0];
  GameOutputSound(GameState, SoundBuffer, 256);
}

game_controller_input *GetController(game_input *Input, int ControllerIndex) {
  Assert(ControllerIndex < ArrayCount(Input->Controllers));
  game_controller_input *Result = &Input->Controllers[ControllerIndex];
  return (Result);
}

} // extern "C"
