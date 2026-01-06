#include "goliath.h"
#include <xlocale/_stdio.h>
// Ensure C linkage for dynamic library export
extern "C" {
#define M_PI 3.1415
#define TILE_MAP_COUNT_X 17
#define TILE_MAP_COUNT_Y 9

uint32 Tiles[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] = {
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 0, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 1},
    {1, 0, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 1},
    {1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 0, 1, 1, 1, 1, 1},
    {1, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0},
    {1, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1},
    {1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
};

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

inline int32 RoundReal32ToInt32(real32 Value) {
  int32 Result = (int32)(Value + 0.5f);
  return Result;
}

inline uint32 RoundReal32ToUInt32(real32 Value) {
  uint32 Result = (uint32)(Value + 0.5f);
  return Result;
}

inline int32 TruncateReal32ToInt32(real32 Value) {
  int32 Result = (int32)Value;
  return Result;
}

struct tile_map {
  real32 UpperLeftX;
  real32 UpperLeftY;
  real32 Width;
  real32 Height;
  int32 CountX;
  int32 CountY;

  uint32 *Tiles;
};

internal_usage bool32 IsTileMapPointEmpty(tile_map *TileMapStruct, real32 TestX,
                                          real32 TestY) {
  bool32 IsEmpty = false;
  int32 PlayerTileX = TruncateReal32ToInt32(
      (TestX - TileMapStruct->UpperLeftX) / TileMapStruct->Width);
  int32 PlayerTileY = TruncateReal32ToInt32(
      (TestY - TileMapStruct->UpperLeftY) / TileMapStruct->Height);

  if ((PlayerTileX >= 0) && (PlayerTileX < TileMapStruct->CountX) &&
      (PlayerTileY >= 0) && (PlayerTileY < TileMapStruct->CountY)) {

    uint32 TileMapValue =
        TileMapStruct->Tiles[PlayerTileY * TileMapStruct->CountX + PlayerTileX];

    IsEmpty = (TileMapValue == 0);
  }
  return IsEmpty;
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

void GameUpdateAndRender(platform_thread_context *ThreadContext,
                         game_memory *GameMemory, game_input *Input,
                         game_offscreen_buffer *Buffer,
                         game_sound_output_buffer *SoundBuffer) {
  game_state *GameState = (game_state *)GameMemory->PermanentStorage;
  tile_map TileMap;

  TileMap.UpperLeftX = -30;
  TileMap.UpperLeftY = 0;
  TileMap.Width = (real32)Buffer->Width / 16.0f;
  TileMap.Height = (real32)Buffer->Height / 9.0f;
  TileMap.CountX = TILE_MAP_COUNT_X;
  TileMap.CountY = TILE_MAP_COUNT_Y;

  TileMap.Tiles = (uint32 *)Tiles;

  if (!GameMemory->IsInitialized) {
    GameState->PlayerX = 100.0f;
    GameState->PlayerY = 100.0f;
    GameMemory->IsInitialized = true;
  }

  DrawRectangle(Buffer, 0.0f, 0.0f, Buffer->Width, Buffer->Height, 1.0f, 1.0f,
                1.0f);

  for (int Row = 0; Row < 9; ++Row) {
    for (int Col = 0; Col < 16; ++Col) {
      uint32 TileID = Tiles[Row][Col];
      real32 MinX = (real32)Col * TileMap.Width;
      real32 MinY = (real32)Row * TileMap.Height;
      real32 MaxX = MinX + TileMap.Width;
      real32 MaxY = MinY + TileMap.Height;
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
  real32 PlayerWidth = 0.75f * TileMap.Width;
  real32 PlayerHeight = 0.75f * TileMap.Height;
  real32 PlayerLeft = GameState->PlayerX - 0.5f * PlayerWidth;
  real32 PlayerTop = GameState->PlayerY - PlayerHeight;

  DrawRectangle(Buffer, PlayerLeft, PlayerTop, PlayerLeft + PlayerWidth,
                PlayerTop + PlayerHeight, PlayerR, PlayerG, PlayerB);

  game_controller_input *Controller = &Input->Controllers[0];
  real32 dPlayerX = 0.0f;
  real32 dPlayerY = 0.0f;

  if (Controller->MoveRight.EndedDown) {
    dPlayerX = 1.0f;
  }
  if (Controller->MoveUp.EndedDown) {
    dPlayerY = -1.0f;
  }
  if (Controller->MoveLeft.EndedDown) {
    dPlayerX = -1.0f;
  }
  if (Controller->MoveDown.EndedDown) {
    dPlayerY = 1.0f;
  }
  dPlayerX *= 20.0f;
  dPlayerY *= 20.0f;

  GameState->PlayerX += Input->dtForFrame * dPlayerX;
  GameState->PlayerY += Input->dtForFrame * dPlayerY;

  real32 NewPlayerX = GameState->PlayerX + Input->dtForFrame * dPlayerX;
  real32 NewPlayerY = GameState->PlayerY + Input->dtForFrame * dPlayerY;

  bool32 IsValid = false;

  GameOutputSound(GameState, SoundBuffer, 256);
}

game_controller_input *GetController(game_input *Input, int ControllerIndex) {
  Assert(ControllerIndex < ArrayCount(Input->Controllers));
  game_controller_input *Result = &Input->Controllers[ControllerIndex];
  return (Result);
}

} // extern "C"
