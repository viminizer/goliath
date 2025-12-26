#include "goliath.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>

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

void GameUpdateAndRender(game_memory *GameMemory, game_input *Input,
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

  game_controller_input *Input0 = &Input->Controllers[0];
  if (Input0->IsAnalog) {
    GameState->ToneHZ = 256 + (int)128.0f * (Input0->EndX);
    GameState->BlueOffset += (int)4.0f * (Input0->EndY);
  } else {
  }

  if (Input0->Down.EndedDown) {
    GameState->GreenOffset += 1;
  }

  GameOutputSound(SoundBuffer, GameState->ToneHZ);
  RenderWeirdGradient(Buffer, GameState->BlueOffset, GameState->GreenOffset);
}
