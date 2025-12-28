#include "sdl_goliath.h"
#include "goliath.h"
#include <cstdio>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <x86intrin.h>

#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

#define MAX_CONTROLLERS 5
global_variable sdl_offscreen_buffer GlobalBackbuffer;

global_variable game_keyboard_state KeyboardState = {};

SDL_GameController *ControllerHandles[MAX_CONTROLLERS];
SDL_Haptic *RumbleHandles[MAX_CONTROLLERS];

inline uint32 SafeTruncateUInt64(uint64 Value) {
  Assert(Value <= 0xFFFFFFFF);
  uint32 FileSize32 = (uint32)Value;
  return FileSize32;
}

internal_usage real32 SDLGetSecondsElapsed(uint64 OldCounter,
                                           uint64 CurrentCounter) {
  return ((real32)(CurrentCounter - OldCounter) /
          (real32)(SDL_GetPerformanceFrequency()));
}

internal_usage int SDLGetWindowRefreshRate(SDL_Window *Window) {
  SDL_DisplayMode Mode;
  int DisplayIndex = SDL_GetWindowDisplayIndex(Window);
  int DefaultRefreshRate = 60;
  if (SDL_GetDesktopDisplayMode(DisplayIndex, &Mode) != 0) {
    return DefaultRefreshRate;
  }
  if (Mode.refresh_rate == 0) {
    return DefaultRefreshRate;
  }
  return Mode.refresh_rate;
}

void DEBUGPlatformFreeFileMemory(void *Memory) { free(Memory); }

debug_read_file_result DEBUGPlatformReadEntireFile(char *Filename) {
  int FileHandle = open(Filename, O_RDONLY);
  printf("FILE HANLE %d\n", FileHandle);
  debug_read_file_result Result = {};
  if (FileHandle < 0) {
    printf("Failed to load file %s\n", Filename);
    return (Result);
  }
  struct stat FileStatus;
  if (fstat(FileHandle, &FileStatus) == -1) {
    close(FileHandle);
    return Result;
  }
  Result.ContentSize = SafeTruncateUInt64(FileStatus.st_size);
  Result.Contents = malloc(Result.ContentSize);
  if (Result.Contents) {
    uint32 BytesToRead = Result.ContentSize;
    uint8 *NextByteLocation = (uint8 *)Result.Contents;
    while (BytesToRead) {
      uint32 BytesRead = read(FileHandle, NextByteLocation, BytesToRead);
      if (BytesRead == -1) {
        free(Result.Contents);
        Result.Contents = 0;
        Result.ContentSize = 0;
        close(FileHandle);
        return Result;
      }
      BytesToRead -= BytesRead;
      NextByteLocation += BytesRead;
    }

  } else {
    DEBUGPlatformFreeFileMemory(Result.Contents);
    Result = {};
  }
  return (Result);
}

internal_usage real32
SDLProcessGameControllerAxisValue(int16 Value, int16 DeadzoneThreshold) {
  real32 Result = 0;

  if (Value < -DeadzoneThreshold) {
    Result =
        (real32)((Value + DeadzoneThreshold) / (32768.0f - DeadzoneThreshold));
  } else if (Value > DeadzoneThreshold) {
    Result =
        (real32)((Value - DeadzoneThreshold) / (32767.0f - DeadzoneThreshold));
  }
  return (Result);
}

bool32 DEBUGPlatformWriteEntireFile(char *Filename, uint32 MemorySize,
                                    void *Memory) {
  int FileHandle =
      open(Filename, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  if (FileHandle == -1) {
    return false;
  }
  uint32 BytesToWrite = MemorySize;
  uint8 *NextByteLocation = (uint8 *)Memory;
  while (BytesToWrite) {
    uint32 BytesWritten = write(FileHandle, NextByteLocation, BytesToWrite);
    if (BytesWritten == -1) {
      close(FileHandle);
      return false;
    }
    BytesToWrite -= BytesWritten;
    NextByteLocation += BytesWritten;
  }
  close(FileHandle);
  return true;
}

sdl_audio_ring_buffer AudioRingBuffer;

internal_usage void SDLAudioCallback(void *UserData, Uint8 *AudioData,
                                     int Length) {
  sdl_audio_ring_buffer *RingBuffer = (sdl_audio_ring_buffer *)UserData;
  int Region1Size = Length;
  int Region2Size = 0;
  if (RingBuffer->PlayCursor + Length > RingBuffer->Size) {
    Region1Size = RingBuffer->Size - RingBuffer->PlayCursor;
    Region2Size = Length - Region1Size;
  }
  memcpy(AudioData, (uint8 *)(RingBuffer->Data) + RingBuffer->PlayCursor,
         Region1Size);
  memcpy(&AudioData[Region1Size], RingBuffer->Data, Region2Size);
  RingBuffer->PlayCursor = (RingBuffer->PlayCursor + Length) % RingBuffer->Size;
  RingBuffer->WriteCursor =
      (RingBuffer->PlayCursor + Length) % RingBuffer->Size;
}

internal_usage void SDLInitAudio(int32 SamplesPerSecond, int32 BufferSize) {
  SDL_AudioSpec AudioSettings = {0};

  AudioSettings.freq = SamplesPerSecond;
  AudioSettings.format = AUDIO_S16LSB;
  AudioSettings.channels = 2;
  AudioSettings.samples = 512;
  AudioSettings.callback = &SDLAudioCallback;
  AudioSettings.userdata = &AudioRingBuffer;

  AudioRingBuffer.Size = BufferSize;
  AudioRingBuffer.Data = calloc(BufferSize, 1);
  AudioRingBuffer.PlayCursor = AudioRingBuffer.WriteCursor = 0;

  SDL_OpenAudio(&AudioSettings, 0);

  printf("Initialised an Audio device at frequency %d Hz, %d Channels, buffer "
         "size %d\n",
         AudioSettings.freq, AudioSettings.channels, AudioSettings.size);

  if (AudioSettings.format != AUDIO_S16LSB) {
    printf("Oops! We didn't get AUDIO_S16LSB as our sample format!\n");
    SDL_CloseAudio();
  }
}

sdl_window_dimension SDLGetWindowDimension(SDL_Window *Window) {
  sdl_window_dimension Result;

  SDL_GetWindowSize(Window, &Result.Width, &Result.Height);

  return (Result);
}

internal_usage void SDLResizeTexture(sdl_offscreen_buffer *Buffer,
                                     SDL_Renderer *Renderer, int Width,
                                     int Height) {
  int BytesPerPixel = 4;
  if (Buffer->Memory) {
    munmap(Buffer->Memory, Buffer->Width * Buffer->Height * BytesPerPixel);
  }
  if (Buffer->Texture) {
    SDL_DestroyTexture(Buffer->Texture);
  }
  Buffer->Texture =
      SDL_CreateTexture(Renderer, SDL_PIXELFORMAT_ARGB8888,
                        SDL_TEXTUREACCESS_STREAMING, Width, Height);
  Buffer->Width = Width;
  Buffer->Height = Height;
  Buffer->Pitch = Width * BytesPerPixel;
  Buffer->Memory =
      mmap(0, Width * Height * BytesPerPixel, PROT_READ | PROT_WRITE,
           MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
}

internal_usage void SDLUpdateWindow(SDL_Window *Window, SDL_Renderer *Renderer,
                                    sdl_offscreen_buffer *Buffer) {
  SDL_UpdateTexture(Buffer->Texture, 0, Buffer->Memory, Buffer->Pitch);
  SDL_RenderCopy(Renderer, Buffer->Texture, 0, 0);
  SDL_RenderPresent(Renderer);
}

bool HandleEvent(SDL_Event *Event) {
  bool ShouldQuit = false;
  switch (Event->type) {
  case SDL_QUIT: {
    printf("SDL_QUIT\n");
    ShouldQuit = true;
  } break;

  case SDL_KEYDOWN:
  case SDL_KEYUP: {
    SDL_Keycode KeyCode = Event->key.keysym.sym;
    bool IsDown = (Event->key.state == SDL_PRESSED);
    if (Event->key.repeat == 0) {
      if (KeyCode == SDLK_w) {
        KeyboardState.KEY_W = IsDown ? true : false;
      } else if (KeyCode == SDLK_a) {
        KeyboardState.KEY_A = IsDown ? true : false;
      } else if (KeyCode == SDLK_s) {
        KeyboardState.KEY_S = IsDown ? true : false;
      } else if (KeyCode == SDLK_d) {
        KeyboardState.KEY_D = IsDown ? true : false;
      } else if (KeyCode == SDLK_q) {
        KeyboardState.LEFT_SHOULDER = IsDown ? true : false;
      } else if (KeyCode == SDLK_e) {
        KeyboardState.RIGHT_SHOULDER = IsDown ? true : false;
      } else if (KeyCode == SDLK_UP) {
        KeyboardState.ARROW_UP = IsDown ? true : false;
      } else if (KeyCode == SDLK_LEFT) {
        KeyboardState.ARROW_LEFT = IsDown ? true : false;
      } else if (KeyCode == SDLK_DOWN) {
        KeyboardState.ARROW_DOWN = IsDown ? true : false;
      } else if (KeyCode == SDLK_RIGHT) {
        KeyboardState.ARROW_RIGHT = IsDown ? true : false;
      } else if (KeyCode == SDLK_ESCAPE) {
        KeyboardState.BACK = IsDown ? true : false; // ESC is back button
      } else if (KeyCode == SDLK_SPACE) {
        KeyboardState.START = IsDown ? true : false; // SPACE is start button
      }
    }

    bool AltKeyWasDown = (Event->key.keysym.mod & KMOD_ALT);
    if (KeyCode == SDLK_F4 && AltKeyWasDown) {
      ShouldQuit = true;
    }

  } break;

  case SDL_WINDOWEVENT: {
    switch (Event->window.event) {
    case SDL_WINDOWEVENT_SIZE_CHANGED: {
      SDL_Window *Window = SDL_GetWindowFromID(Event->window.windowID);
      SDL_Renderer *Renderer = SDL_GetRenderer(Window);
      printf("SDL_WINDOWEVENT_SIZE_CHANGED (%d, %d)\n", Event->window.data1,
             Event->window.data2);
    } break;

    case SDL_WINDOWEVENT_FOCUS_GAINED: {
      printf("SDL_WINDOWEVENT_FOCUS_GAINED\n");
    } break;

    case SDL_WINDOWEVENT_EXPOSED: {
      SDL_Window *Window = SDL_GetWindowFromID(Event->window.windowID);
      SDL_Renderer *Renderer = SDL_GetRenderer(Window);
      SDLUpdateWindow(Window, Renderer, &GlobalBackbuffer);
    } break;
    }
  } break;
  }
  return (ShouldQuit);
}

internal_usage void SDLFillSoundBuffer(sdl_sound_output *SoundOutput,
                                       int ByteToLock, int BytesToWrite,
                                       game_sound_output_buffer *SoundBuffer) {
  int16_t *Samples = SoundBuffer->Samples;
  void *Region1 = (uint8 *)AudioRingBuffer.Data + ByteToLock;
  int Region1Size = BytesToWrite;
  if (Region1Size + ByteToLock > SoundOutput->SecondaryBufferSize) {
    Region1Size = SoundOutput->SecondaryBufferSize - ByteToLock;
  }
  void *Region2 = AudioRingBuffer.Data;
  int Region2Size = BytesToWrite - Region1Size;
  int Region1SampleCount = Region1Size / SoundOutput->BytesPerSample;
  int16 *SampleOut = (int16 *)Region1;
  for (int SampleIndex = 0; SampleIndex < Region1SampleCount; ++SampleIndex) {
    *SampleOut++ = *Samples++;
    *SampleOut++ = *Samples++;

    ++SoundOutput->RunningSampleIndex;
  }

  int Region2SampleCount = Region2Size / SoundOutput->BytesPerSample;
  SampleOut = (int16 *)Region2;
  for (int SampleIndex = 0; SampleIndex < Region2SampleCount; ++SampleIndex) {
    *SampleOut++ = *Samples++;
    *SampleOut++ = *Samples++;
    ++SoundOutput->RunningSampleIndex;
  }
}

internal_usage void SDLOpenGameControllers() {
  int MaxJoysticks = SDL_NumJoysticks();
  int ControllerIndex = 0;
  for (int JoystickIndex = 0; JoystickIndex < MaxJoysticks; ++JoystickIndex) {
    if (!SDL_IsGameController(JoystickIndex)) {
      continue;
    }
    if (ControllerIndex >= MAX_CONTROLLERS) {
      break;
    }
    ControllerHandles[ControllerIndex] = SDL_GameControllerOpen(JoystickIndex);
    SDL_Joystick *JoystickHandle =
        SDL_GameControllerGetJoystick(ControllerHandles[ControllerIndex]);
    RumbleHandles[ControllerIndex] = SDL_HapticOpenFromJoystick(JoystickHandle);
    if (SDL_HapticRumbleInit(RumbleHandles[ControllerIndex]) != 0) {
      SDL_HapticClose(RumbleHandles[ControllerIndex]);
      RumbleHandles[ControllerIndex] = 0;
    }
    ControllerIndex++;
  }
}

internal_usage void SDLProcessGameControllerButton(game_button_state *OldState,
                                                   game_button_state *NewState,
                                                   bool32 IsDown) {
  // Assert(NewState->EndedDown != IsDown);
  NewState->EndedDown = IsDown;
  NewState->HalfTransitionCount +=
      ((NewState->EndedDown == OldState->EndedDown) ? 0 : 1);
}

internal_usage void SDLCloseGameControllers() {
  for (int ControllerIndex = 0; ControllerIndex < MAX_CONTROLLERS;
       ++ControllerIndex) {
    if (ControllerHandles[ControllerIndex]) {
      if (RumbleHandles[ControllerIndex])
        SDL_HapticClose(RumbleHandles[ControllerIndex]);
      SDL_GameControllerClose(ControllerHandles[ControllerIndex]);
    }
  }
}

int main(int argc, char *argv[]) {
  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC |
           SDL_INIT_AUDIO);
  uint64 PerfCountFrequency = SDL_GetPerformanceFrequency();
  Assert(PerfCountFrequency > 0);
  SDLOpenGameControllers();
  SDL_Window *Window =
      SDL_CreateWindow("Goliath Game Engine", SDL_WINDOWPOS_UNDEFINED,
                       SDL_WINDOWPOS_UNDEFINED, 640, 480, SDL_WINDOW_RESIZABLE);
  if (Window) {
    SDL_Renderer *Renderer = SDL_CreateRenderer(Window, -1, 0);

    printf("Refresh rate is %d Hz\n", SDLGetWindowRefreshRate(Window));

    int GameUpdateHz = 30;

    real32 TargetSecondsPerFrame = 1.0f / (real32)GameUpdateHz;

    if (Renderer) {
      bool Running = true;
      sdl_window_dimension Dimension = SDLGetWindowDimension(Window);
      SDLResizeTexture(&GlobalBackbuffer, Renderer, Dimension.Width,
                       Dimension.Height);

      game_input Input[2] = {};
      game_input *NewInput = &Input[0];
      game_input *OldInput = &Input[1];

      sdl_sound_output SoundOutput = {};
      SoundOutput.SamplesPerSecond = 48000;
      SoundOutput.RunningSampleIndex = 0;
      SoundOutput.BytesPerSample = sizeof(int16) * 2;
      SoundOutput.SecondaryBufferSize =
          SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample;
      SoundOutput.LatencySampleCount = SoundOutput.SamplesPerSecond / 15;
      // Open our audio device:
      SDLInitAudio(48000, SoundOutput.SecondaryBufferSize);
      // NOTE: calloc() allocates memory and clears it to zero. It accepts the
      // number of things being allocated and their size.
      int16 *Samples = (int16 *)calloc(SoundOutput.SamplesPerSecond,
                                       SoundOutput.BytesPerSample);
      SDL_PauseAudio(0);

#if GOLIATH_INTERNAL
      void *BaseAddress = (void *)Terabytes(2);
#else
      void *BaseAddress = (void *)(0);
#endif

      game_memory GameMemory = {};
      GameMemory.PermanentStorageSize = Megabytes(64);
      GameMemory.TransientStorageSize = Gigabytes(4);
      uint64 TotalStorageSize =
          GameMemory.PermanentStorageSize + GameMemory.TransientStorageSize;
      GameMemory.PermanentStorage =
          mmap(BaseAddress, TotalStorageSize, PROT_READ | PROT_WRITE,
               MAP_ANON | MAP_PRIVATE, -1, 0);
      Assert(GameMemory.PermanentStorage);
      GameMemory.TransientStorage = (uint8 *)(GameMemory.PermanentStorage) +
                                    GameMemory.PermanentStorageSize;
      if (Samples && GameMemory.PermanentStorage) {
        uint64 LastCounter = SDL_GetPerformanceCounter();
        uint64 LastCycleCount = _rdtsc();

        while (Running) {

          SDL_Event Event;
          while (SDL_PollEvent(&Event)) {
            if (HandleEvent(&Event)) {
              Running = false;
            }
          }
          // Poll the keyboard
          {
            game_controller_input *OldController = GetController(OldInput, 0);
            game_controller_input *NewController = GetController(NewInput, 0);

            NewController->IsConnected = true;
            NewController->IsAnalog = false;
            NewController->StickAverageX = 0.0f;
            NewController->StickAverageY = 0.0f;

            SDLProcessGameControllerButton(&(OldController->MoveUp),
                                           &(NewController->MoveUp),
                                           KeyboardState.KEY_W);

            SDLProcessGameControllerButton(&(OldController->MoveDown),
                                           &(NewController->MoveDown),
                                           KeyboardState.KEY_S);

            SDLProcessGameControllerButton(&(OldController->MoveLeft),
                                           &(NewController->MoveLeft),
                                           KeyboardState.KEY_A);

            SDLProcessGameControllerButton(&(OldController->MoveRight),
                                           &(NewController->MoveRight),
                                           KeyboardState.KEY_D);
          }

          // Poll our controllers for input.
          for (int ControllerIndex = 1; ControllerIndex < MAX_CONTROLLERS;
               ++ControllerIndex) {

            if (ControllerHandles[ControllerIndex] != 0 &&
                SDL_GameControllerGetAttached(
                    ControllerHandles[ControllerIndex])) {

              game_controller_input *OldController =
                  GetController(OldInput, ControllerIndex);
              game_controller_input *NewController =
                  GetController(NewInput, ControllerIndex);

              NewController->IsConnected = true;

              // TODO: Do something with the DPad, Start and Selected?
              bool Up = SDL_GameControllerGetButton(
                  ControllerHandles[ControllerIndex],
                  SDL_CONTROLLER_BUTTON_DPAD_UP);
              bool Down = SDL_GameControllerGetButton(
                  ControllerHandles[ControllerIndex],
                  SDL_CONTROLLER_BUTTON_DPAD_DOWN);
              bool Left = SDL_GameControllerGetButton(
                  ControllerHandles[ControllerIndex],
                  SDL_CONTROLLER_BUTTON_DPAD_LEFT);
              bool Right = SDL_GameControllerGetButton(
                  ControllerHandles[ControllerIndex],
                  SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
              bool Start = SDL_GameControllerGetButton(
                  ControllerHandles[ControllerIndex],
                  SDL_CONTROLLER_BUTTON_START);
              bool Back = SDL_GameControllerGetButton(
                  ControllerHandles[ControllerIndex],
                  SDL_CONTROLLER_BUTTON_BACK);

              SDLProcessGameControllerButton(
                  &(OldController->LeftShoulder),
                  &(NewController->LeftShoulder),
                  SDL_GameControllerGetButton(
                      ControllerHandles[ControllerIndex],
                      SDL_CONTROLLER_BUTTON_LEFTSHOULDER));

              SDLProcessGameControllerButton(
                  &(OldController->RightShoulder),
                  &(NewController->RightShoulder),
                  SDL_GameControllerGetButton(
                      ControllerHandles[ControllerIndex],
                      SDL_CONTROLLER_BUTTON_RIGHTSHOULDER));

              SDLProcessGameControllerButton(
                  &(OldController->ActionDown), &(NewController->ActionDown),
                  SDL_GameControllerGetButton(
                      ControllerHandles[ControllerIndex],
                      SDL_CONTROLLER_BUTTON_A));

              SDLProcessGameControllerButton(
                  &(OldController->ActionRight), &(NewController->ActionRight),
                  SDL_GameControllerGetButton(
                      ControllerHandles[ControllerIndex],
                      SDL_CONTROLLER_BUTTON_B));

              SDLProcessGameControllerButton(
                  &(OldController->ActionLeft), &(NewController->ActionLeft),
                  SDL_GameControllerGetButton(
                      ControllerHandles[ControllerIndex],
                      SDL_CONTROLLER_BUTTON_X));

              SDLProcessGameControllerButton(
                  &(OldController->ActionUp), &(NewController->ActionUp),
                  SDL_GameControllerGetButton(
                      ControllerHandles[ControllerIndex],
                      SDL_CONTROLLER_BUTTON_Y));

              NewController->StickAverageX = SDLProcessGameControllerAxisValue(
                  SDL_GameControllerGetAxis(ControllerHandles[ControllerIndex],
                                            SDL_CONTROLLER_AXIS_LEFTX),
                  1);
              NewController->StickAverageY = -SDLProcessGameControllerAxisValue(
                  SDL_GameControllerGetAxis(ControllerHandles[ControllerIndex],
                                            SDL_CONTROLLER_AXIS_LEFTY),
                  1);

              if (NewController->StickAverageX != 0.0f ||
                  NewController->StickAverageY != 0.0f) {
                NewController->IsAnalog = true;
              }

              if (SDL_GameControllerGetButton(
                      ControllerHandles[ControllerIndex],
                      SDL_CONTROLLER_BUTTON_DPAD_UP)) {
                NewController->StickAverageY = 1.0f;
                NewController->IsAnalog = false;
              }

              if (SDL_GameControllerGetButton(
                      ControllerHandles[ControllerIndex],
                      SDL_CONTROLLER_BUTTON_DPAD_DOWN)) {
                NewController->StickAverageY = -1.0f;
                NewController->IsAnalog = false;
              }

              if (SDL_GameControllerGetButton(
                      ControllerHandles[ControllerIndex],
                      SDL_CONTROLLER_BUTTON_DPAD_LEFT)) {
                NewController->StickAverageX = -1.0f;
                NewController->IsAnalog = false;
              }

              if (SDL_GameControllerGetButton(
                      ControllerHandles[ControllerIndex],
                      SDL_CONTROLLER_BUTTON_DPAD_RIGHT)) {
                NewController->StickAverageX = 1.0f;
                NewController->IsAnalog = false;
              }

              real32 Threshold = 0.5f;

              SDLProcessGameControllerButton(
                  &(OldController->MoveLeft), &(NewController->MoveLeft),
                  NewController->StickAverageX < -Threshold);

              SDLProcessGameControllerButton(
                  &(OldController->MoveRight), &(NewController->MoveRight),
                  NewController->StickAverageX > Threshold);

              SDLProcessGameControllerButton(
                  &(OldController->MoveUp), &(NewController->MoveUp),
                  NewController->StickAverageY > Threshold);

              SDLProcessGameControllerButton(
                  &(OldController->MoveDown), &(NewController->MoveDown),
                  NewController->StickAverageY < -Threshold);

            } else {
              // TODO: This controller is not plugged in.
            }
          }

          // Sound output test
          SDL_LockAudio();
          int ByteToLock =
              (SoundOutput.RunningSampleIndex * SoundOutput.BytesPerSample) %
              SoundOutput.SecondaryBufferSize;
          int TargetCursor =
              ((AudioRingBuffer.PlayCursor +
                (SoundOutput.LatencySampleCount * SoundOutput.BytesPerSample)) %
               SoundOutput.SecondaryBufferSize);
          int BytesToWrite;
          if (ByteToLock > TargetCursor) {
            BytesToWrite = (SoundOutput.SecondaryBufferSize - ByteToLock);
            BytesToWrite += TargetCursor;
          } else {
            BytesToWrite = TargetCursor - ByteToLock;
          }

          SDL_UnlockAudio();

          game_sound_output_buffer SoundBuffer = {};
          SoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
          SoundBuffer.SampleCount = BytesToWrite / SoundOutput.BytesPerSample;
          SoundBuffer.Samples = Samples;

          game_offscreen_buffer Buffer = {};
          Buffer.Memory = GlobalBackbuffer.Memory;
          Buffer.Width = GlobalBackbuffer.Width;
          Buffer.Height = GlobalBackbuffer.Height;
          Buffer.Pitch = GlobalBackbuffer.Pitch;

          GameUpdateAndRender(&GameMemory, NewInput, &Buffer, &SoundBuffer);

          SDLFillSoundBuffer(&SoundOutput, ByteToLock, BytesToWrite,
                             &SoundBuffer);
          game_input *Temp = NewInput;
          NewInput = OldInput;
          OldInput = Temp;

          SDLUpdateWindow(Window, Renderer, &GlobalBackbuffer);

          // Frame rate limiting
          real32 SecondsElapsed =
              SDLGetSecondsElapsed(LastCounter, SDL_GetPerformanceCounter());
          if (SecondsElapsed < TargetSecondsPerFrame) {
            real32 SecondsLeftInFrame = TargetSecondsPerFrame - SecondsElapsed;
            uint32 TimeToSleep =
                (uint32)((SecondsLeftInFrame * 1000.0f) - 1.0f);

            if (TimeToSleep > 0) {
              SDL_Delay(TimeToSleep);
            }

            while (
                SDLGetSecondsElapsed(LastCounter, SDL_GetPerformanceCounter()) <
                TargetSecondsPerFrame) {
              // Busy-wait for precision
            }
          }

          // Measure frame time (always, regardless of whether we delayed)
          uint64 EndCycleCount = _rdtsc();
          uint64 EndCounter = SDL_GetPerformanceCounter();

          uint64 CounterElapsed = EndCounter - LastCounter;
          if (CounterElapsed == 0) {
            CounterElapsed = 1;
          }
          uint64 CyclesElapsed = EndCycleCount - LastCycleCount;

          real64 MSPerFrame = (((1000.0f * (real64)CounterElapsed) /
                                (real64)PerfCountFrequency));
          real64 FPS = (real64)PerfCountFrequency / (real64)CounterElapsed;
          real64 MCPF = ((real64)CyclesElapsed / (1000.0f * 1000.0f));

          printf("%.02fms/f, %.02f fps, %.02fmc/f\n", MSPerFrame, FPS, MCPF);

          LastCycleCount = EndCycleCount;
          LastCounter = EndCounter;
        }
      } else {
        // TODO(casey): Logging
      }
    } else {
      // TODO(casey): Logging
    }
  }

  SDLCloseGameControllers();
  SDL_Quit();
  return (0);
}
