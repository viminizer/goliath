#include "goliath.h"

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

void GameUpdateAndRender(game_offscreen_buffer *Buffer, int BlueOffset,
                         int GreenOffset) {
  RenderWeirdGradient(Buffer, BlueOffset, GreenOffset);
}
