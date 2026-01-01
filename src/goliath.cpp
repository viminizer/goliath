#include "goliath.h"
#include <xlocale/_stdio.h>
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

internal_usage uint32 RoundReal32ToUInt32(real32 Value) {
  uint32 Result = (uint32)(Value + 0.5f);
  return Result;
}

internal_usage void DrawRectangle(game_offscreen_buffer *Buffer,
                                  real32 RealMinX, real32 RealMinY,
                                  real32 RealMaxX, real32 RealMaxY, real32 R,
                                  real32 G, real32 B) {

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

  uint32 Color = (RoundReal32ToUInt32(R * 255.0f) << 16) |
                 (RoundReal32ToUInt32(G * 255.0f) << 8) |
                 RoundReal32ToUInt32(B * 255.0f) << 0;

  uint8 *Row = (uint8 *)Buffer->Memory + MinY * Buffer->Pitch;
  for (int Y = MinY; Y < MaxY; ++Y) {
    uint32 *Pixel = (uint32 *)Row + MinX;
    for (int X = MinX; X < MaxX; ++X) {
      *Pixel++ = Color;
    }
    Row += Buffer->Pitch;
  }
}
uint32 TileMap[9][16] = {
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
};

void GameUpdateAndRender(platform_thread_context *ThreadContext,
                         game_memory *GameMemory, game_input *Input,
                         game_offscreen_buffer *Buffer,
                         game_sound_output_buffer *SoundBuffer) {
  game_state *GameState = (game_state *)GameMemory->PermanentStorage;
  if (!GameMemory->IsInitialized) {
    GameState->PlayerX = 100.0f;
    GameState->PlayerY = 100.0f;
    GameMemory->IsInitialized = true;
  }

  real32 UpperLeftX = 0;
  real32 UpperLeftY = 0;
  real32 TileWidth = (real32)Buffer->Width / 16.0f;
  real32 TileHeight = (real32)Buffer->Height / 9.0f;

  DrawRectangle(Buffer, 0.0f, 0.0f, Buffer->Width, Buffer->Height, 1.0f, 1.0f,
                1.0f);

  for (int Row = 0; Row < 9; ++Row) {
    for (int Col = 0; Col < 16; ++Col) {
      uint32 TileID = TileMap[Row][Col];
      real32 MinX = (real32)Col * TileWidth;
      real32 MinY = (real32)Row * TileHeight;
      real32 MaxX = MinX + TileWidth;
      real32 MaxY = MinY + TileHeight;
      real32 Gray = 1.0f;
      if (TileID == 1) {
        Gray = 0.3f;
      }

      DrawRectangle(Buffer, MinX, MinY, MaxX, MaxY, Gray, Gray, Gray);
    }
  }

  real32 PlayerR = 1.0f;
  real32 PlayerG = 1.0f;
  real32 PlayerB = 0.0f;
  real32 PlayerWidth = 0.75f * TileWidth;
  real32 PlayerHeight = 0.75f * TileHeight;
  real32 PlayerLeft = GameState->PlayerX - 0.5f * PlayerWidth;
  real32 PlayerTop = GameState->PlayerY - PlayerHeight;

  DrawRectangle(Buffer, PlayerLeft, PlayerTop, PlayerLeft + PlayerWidth,
                PlayerTop + PlayerHeight, PlayerR, PlayerG, PlayerB);

  game_controller_input *Controller = &Input->Controllers[0];
  real32 dPlayerX = 0.0f;
  real32 dPlayerY = 0.0f;

  if (Controller->MoveRight.EndedDown) {
    printf("MOVE RIGHT IS DOWN\n");
    dPlayerX = 10.0f;
  }
  if (Controller->MoveUp.EndedDown) {
    printf("MOVE UP IS DOWN\n");
    dPlayerY = -10.0f;
  }
  if (Controller->MoveLeft.EndedDown) {
    printf("MOVE LEFT IS DOWN\n");
    dPlayerX = -10.0f;
  }
  if (Controller->MoveDown.EndedDown) {
    printf("MOVE DOWN IS DOWN\n");
    dPlayerY = 10.0f;
  }
  dPlayerX *= 20.0f;
  dPlayerY *= 20.0f;

  GameState->PlayerX += Input->dtForFrame * dPlayerX;
  GameState->PlayerY += Input->dtForFrame * dPlayerY;

  GameOutputSound(GameState, SoundBuffer, 256);
}

game_controller_input *GetController(game_input *Input, int ControllerIndex) {
  Assert(ControllerIndex < ArrayCount(Input->Controllers));
  game_controller_input *Result = &Input->Controllers[ControllerIndex];
  return (Result);
}

} // extern "C"
