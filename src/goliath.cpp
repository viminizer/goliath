#include "goliath.h"
#include <cmath>

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
  for (int y = 0; y < Buffer->Height; ++y) {
    uint8 *byteInPixel = (uint8 *)Row;
    for (int x = 0; x < Buffer->Width; ++x) {
      // red
      *byteInPixel = 0;
      ++byteInPixel;
      // green
      *byteInPixel = (uint8)y;
      ++byteInPixel;
      // blue
      *byteInPixel = (uint8)x + (uint8)BlueOffset;
      ++byteInPixel;
      // alpha
      *byteInPixel = 255;
      ++byteInPixel;
    }
    Row += Buffer->Pitch;
  }
}

void GameUpdateAndRender(game_input *Input, game_offscreen_buffer *Buffer,
                         game_sound_output_buffer *SoundBuffer) {
  local_persist int BlueOffset = 0;
  local_persist int GreenOffset = 0;
  local_persist int ToneHZ = 256;

  game_controller_input *Input0 = &Input->Controllers[0];
  if (Input0->IsAnalog) {
    ToneHZ = 256 + (int)128.0f * (Input0->EndX);
    BlueOffset += (int)4.0f * (Input0->EndY);
  } else {
  }

  if (Input0->Down.EndedDown) {
    GreenOffset += 1;
  }

  GameOutputSound(SoundBuffer, ToneHZ);
  RenderWeirdGradient(Buffer, BlueOffset, GreenOffset);
}
