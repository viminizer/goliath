#include "goliath.h"
#include <cmath>
#include <cstdio>

// Ensure C linkage for dynamic library export
extern "C" {

internal_usage void GameOutputSound(game_sound_output_buffer *SoundBuffer,
                                    int ToneHZ) {
  local_persist real32 tSine;
  int16 ToneVolume = 3000;
  int WavePeriod = SoundBuffer->SamplesPerSecond / ToneHZ;
  int16 *SampleOut = SoundBuffer->Samples;

  for (int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount;
       ++SampleIndex) {
    real32 SineValue = sinf(tSine);
    int16 SampleValue = (int16)(SineValue * ToneVolume);
    *SampleOut++ = SampleValue;
    *SampleOut++ = SampleValue;
    tSine += 2.0f * M_PI * 1.0f / (real32)WavePeriod;
  }
}

internal_usage void RenderWeirdGradient(game_offscreen_buffer *Buffer,
                                        int BlueOffset, int GreenOffset) {
  uint8 *Row = (uint8 *)Buffer->Memory;
  for (int Y = 0; Y < Buffer->Height; ++Y) {
    uint32 *Pixel = (uint32 *)Row;
    for (int X = 0; X < Buffer->Width; ++X) {
      uint8 Blue = (X + BlueOffset);
      uint8 Green = (Y + GreenOffset);
      *Pixel++ = ((Green << 8) | Blue);
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
    const char *HOME = getenv("HOME");
    printf("%s", HOME);
    char *Filename = (char *)__FILE__;
    char Outfile[1024];
    snprintf(Outfile, sizeof(Outfile),
             "%s/Desktop/github/handmade/goliath/out.txt", HOME);

    debug_read_file_result File = DEBUGPlatformReadEntireFile(Filename);
    if (File.Contents) {
      DEBUGPlatformWriteEntireFile(Outfile, File.ContentSize, File.Contents);
      DEBUGPlatformFreeFileMemory(File.Contents);
    }
    GameState->ToneHZ = 256;
    GameState->BlueOffset = 0;
    GameState->GreenOffset = 0;
    GameMemory->IsInitialized = true;
  }

  game_controller_input *Controller = &Input->Controllers[0];

  // Test ExecutableReloaded flag
  if (Input->ExecutableReloaded) {
    printf("Game code detected hot reload! Reinitializing if needed...\n");
  }

  // Test mouse input
  if (Input->MouseButtons[0].EndedDown) {
    printf("Mouse clicked at (%d, %d)\n", Input->MouseX, Input->MouseY);
  }

  if (Controller->MoveRight.EndedDown) {
    GameState->BlueOffset += 2000; // HOT RELOAD: Testing with 500!
  }

  if (Controller->MoveLeft.EndedDown) {
    GameState->BlueOffset -= 5000; // HOT RELOAD: Testing with 500!
  }

  GameOutputSound(SoundBuffer, GameState->ToneHZ);
  RenderWeirdGradient(Buffer, GameState->BlueOffset, GameState->GreenOffset);
}

game_controller_input *GetController(game_input *Input, int ControllerIndex) {
  Assert(ControllerIndex < ArrayCount(Input->Controllers));
  game_controller_input *Result = &Input->Controllers[ControllerIndex];
  return (Result);
}

} // extern "C"
