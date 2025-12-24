
#include "goliath.h"
struct MacOSSoundOutput {
  int samplesPerSecond;
  int bytesPerSample;
  real32 tSine;
  uint32 runningSampleIndex;
  uint32 bufferSize;
  uint32 writeCursor;
  uint32 playCursor;
  void *data;
};
