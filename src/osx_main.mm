#include "osx_main.h"
#include "goliath_types.h"
#include <AudioToolbox/AudioToolbox.h>
#include <IOKit/hid/IOHIDLib.h>

global_variable float GlobalRenderWidth = 1024;
global_variable float GlobalRenderHeight = 768;

global_variable bool Running = true;

typedef struct macos_offscreen_buffer {
  uint8 *memory;
  int bitmapWidth;
  int bitmapHeight;
  int bytesPerPixel = 4;
  int pitch;
} OffscreenBuffer;

OffscreenBuffer bufferStruct;
OffscreenBuffer *buffer = &bufferStruct;

struct macos_window_dimension {
  int width;
  int height;
};

global_variable int offsetX = 0;
global_variable int offsetY = 0;

void macOSRefreshBuffer(NSWindow *window, OffscreenBuffer *buffer) {
  if (buffer->memory) {
    free(buffer->memory);
  }
  buffer->bitmapWidth = window.contentView.bounds.size.width;
  buffer->bitmapHeight = window.contentView.bounds.size.height;
  buffer->pitch = buffer->bitmapWidth * buffer->bytesPerPixel;
  buffer->memory = (uint8 *)malloc(buffer->pitch * buffer->bitmapHeight);
}

void renderWeirdGradient(OffscreenBuffer *buffer) {
  int width = buffer->bitmapWidth;
  int height = buffer->bitmapHeight;
  uint8 *row = (uint8 *)buffer->memory;

  for (int y = 0; y < height; ++y) {

    uint8 *byteInPixel = (uint8 *)row;

    for (int x = 0; x < width; ++x) {
      // red
      *byteInPixel = 0;
      ++byteInPixel;

      // green
      *byteInPixel = (uint8)y;
      ++byteInPixel;

      // blue
      *byteInPixel = (uint8)x + (uint8)offsetX;
      ++byteInPixel;

      // alpha
      *byteInPixel = 255;
      ++byteInPixel;
    }
    row += buffer->pitch;
  }
}

void macOSRedrawBuffer(NSWindow *window, OffscreenBuffer *buffer) {
  @autoreleasepool {
    NSBitmapImageRep *rep = [[[NSBitmapImageRep alloc]
        initWithBitmapDataPlanes:&buffer->memory
                      pixelsWide:buffer->bitmapWidth
                      pixelsHigh:buffer->bitmapHeight
                   bitsPerSample:8
                 samplesPerPixel:4
                        hasAlpha:YES
                        isPlanar:NO
                  colorSpaceName:NSDeviceRGBColorSpace
                     bytesPerRow:buffer->pitch
                    bitsPerPixel:buffer->bytesPerPixel * 8] autorelease];
    NSSize imageSize = NSMakeSize(buffer->bitmapWidth, buffer->bitmapHeight);
    NSImage *image = [[[NSImage alloc] initWithSize:imageSize] autorelease];
    [image addRepresentation:rep];
    window.contentView.layer.contents = image;
  }
}

@interface GoliathMainWindowDelegate : NSObject <NSWindowDelegate>
@end

@implementation GoliathMainWindowDelegate

- (void)windowWillClose:(id)sender {
  Running = false;
}

- (void)windowDidResize:(NSNotification *)notification {
  NSWindow *window = (NSWindow *)notification.object;
  macOSRefreshBuffer(window, buffer);
  renderWeirdGradient(buffer);
  macOSRedrawBuffer(window, buffer);
}

@end

@interface OSXGoliathController : NSObject

// DPAD
@property NSInteger dpadX;
@property NSInteger dpadY;

// ABXY
@property BOOL buttonAState;
@property BOOL buttonBState;
@property BOOL buttonXState;
@property BOOL buttonYState;

// shoulder buttons
@property BOOL buttonLeftShoulderState;
@property BOOL buttonRightShoulderState;

@end

global_variable IOHIDManagerRef HIDManager = NULL;
global_variable OSXGoliathController *connectedController = nil;
global_variable OSXGoliathController *keyboardController = nil;

@interface GoliathKeyIgnoringWindow : NSWindow
@end

@implementation GoliathKeyIgnoringWindow

- (void)keyDown:(NSEvent *)event {
}
@end

@implementation OSXGoliathController {
  // DPAD
  CFIndex _dpadLUsageID;
  CFIndex _dpadRUsageID;
  CFIndex _dpadDUsageID;
  CFIndex _dpadUUsageID;

  // ABXY
  CFIndex _buttonAUsageID;
  CFIndex _buttonBUsageID;
  CFIndex _buttonXUsageID;
  CFIndex _buttonYUsageID;

  // shoulder
  CFIndex _lShoulderUsageID;
  CFIndex _rShoulderUsageID;
}

static void macOSInitGameControllers() {
  HIDManager = IOHIDManagerCreate(kCFAllocatorDefault, 0);
  connectedController = [[OSXGoliathController alloc] init];
  keyboardController = [[OSXGoliathController alloc] init];

  if (IOHIDManagerOpen(HIDManager, kIOHIDOptionsTypeNone) != kIOReturnSuccess) {
    NSLog(@"Error Initializing OSX Goliath Controllers");
    return;
  }

  IOHIDManagerRegisterDeviceMatchingCallback(HIDManager, controllerConnected,
                                             NULL);
  IOHIDManagerSetDeviceMatchingMultiple(HIDManager, (__bridge CFArrayRef) @[
    @{
      @(kIOHIDDeviceUsagePageKey) : @(kHIDPage_GenericDesktop),
      @(kIOHIDDeviceUsageKey) : @(kHIDUsage_GD_GamePad)
    },
    @{
      @(kIOHIDDeviceUsagePageKey) : @(kHIDPage_GenericDesktop),
      @(kIOHIDDeviceUsageKey) : @(kHIDUsage_GD_MultiAxisController)
    },
  ]);

  IOHIDManagerScheduleWithRunLoop(HIDManager, CFRunLoopGetMain(),
                                  kCFRunLoopDefaultMode);
  NSLog(@"OSXGoliath controller initialized.");
}

const unsigned short leftArrowKeyCode = 0x7B;
const unsigned short rightArrowKeyCode = 0x7C;
const unsigned short downArrowKeyCode = 0x7D;
const unsigned short upArrowKeyCode = 0x7E;
const unsigned short aKeyCode = 0x00;
const unsigned short sKeyCode = 0x01;
const unsigned short dKeyCode = 0x02;
const unsigned short fKeyCode = 0x03;
const unsigned short qKeyCode = 0x0C;
const unsigned short rKeyCode = 0x0F;

static void updateKeyboardControllerWith(NSEvent *event) {
  switch ([event type]) {
  case NSEventTypeKeyDown:
    if (event.keyCode == leftArrowKeyCode && keyboardController.dpadX != 1) {
      keyboardController.dpadX = -1;
      break;
    }

    if (event.keyCode == rightArrowKeyCode && keyboardController.dpadX != -1) {
      keyboardController.dpadX = 1;
      break;
    }

    if (event.keyCode == downArrowKeyCode && keyboardController.dpadY != -1) {
      keyboardController.dpadY = 1;
      break;
    }

    if (event.keyCode == upArrowKeyCode && keyboardController.dpadY != 1) {
      keyboardController.dpadY = -1;
      break;
    }

    if (event.keyCode == aKeyCode) {
      keyboardController.buttonAState = 1;
      break;
    }

    if (event.keyCode == sKeyCode) {
      keyboardController.buttonBState = 1;
      break;
    }

    if (event.keyCode == dKeyCode) {
      keyboardController.buttonXState = 1;
      break;
    }

    if (event.keyCode == fKeyCode) {
      keyboardController.buttonYState = 1;
      break;
    }

    if (event.keyCode == qKeyCode) {
      keyboardController.buttonLeftShoulderState = 1;
      break;
    }

    if (event.keyCode == rKeyCode) {
      keyboardController.buttonRightShoulderState = 1;
      break;
    }

  case NSEventTypeKeyUp:
    if (event.keyCode == leftArrowKeyCode && keyboardController.dpadX == -1) {
      keyboardController.dpadX = 0;
      break;
    }

    if (event.keyCode == rightArrowKeyCode && keyboardController.dpadX == 1) {
      keyboardController.dpadX = 0;
      break;
    }

    if (event.keyCode == downArrowKeyCode && keyboardController.dpadY == 1) {
      keyboardController.dpadY = 0;
      break;
    }

    if (event.keyCode == upArrowKeyCode && keyboardController.dpadY == -1) {
      keyboardController.dpadY = 0;
      break;
    }

    if (event.keyCode == aKeyCode) {
      keyboardController.buttonAState = 0;
      break;
    }

    if (event.keyCode == sKeyCode) {
      keyboardController.buttonBState = 0;
      break;
    }

    if (event.keyCode == dKeyCode) {
      keyboardController.buttonXState = 0;
      break;
    }

    if (event.keyCode == fKeyCode) {
      keyboardController.buttonYState = 0;
      break;
    }

    if (event.keyCode == qKeyCode) {
      keyboardController.buttonLeftShoulderState = 0;
      break;
    }

    if (event.keyCode == rKeyCode) {
      keyboardController.buttonRightShoulderState = 0;
      break;
    }

  default:
    break;
  }
}

static void controllerConnected(void *context, IOReturn result, void *sender,
                                IOHIDDeviceRef device) {
  if (result != kIOReturnSuccess) {
    return;
  }
  NSUInteger vendorID = [(__bridge NSNumber *)IOHIDDeviceGetProperty(
      device, CFSTR(kIOHIDVendorIDKey)) unsignedIntegerValue];
  NSUInteger productID = [(__bridge NSNumber *)IOHIDDeviceGetProperty(
      device, CFSTR(kIOHIDProductIDKey)) unsignedIntegerValue];

  OSXGoliathController *controller = [[OSXGoliathController alloc] init];

  if (vendorID == 0x054C && productID == 0x5C4) {
    NSLog(@"Sony Dualshock 4 detected.");
    controller->_buttonAUsageID = 0x02;
    controller->_buttonBUsageID = 0x03;
    controller->_buttonXUsageID = 0x01;
    controller->_buttonYUsageID = 0x04;
    controller->_lShoulderUsageID = 0x05;
    controller->_rShoulderUsageID = 0x06;
  }
  IOHIDDeviceRegisterInputValueCallback(device, controllerInput,
                                        (__bridge void *)controller);
  IOHIDDeviceSetInputValueMatchingMultiple(device, (__bridge CFArrayRef) @[
    @{@(kIOHIDElementUsagePageKey) : @(kHIDPage_GenericDesktop)},
    @{@(kIOHIDElementUsagePageKey) : @(kHIDPage_Button)},
  ]);

  connectedController = controller;
}

static void controllerInput(void *context, IOReturn result, void *sender,
                            IOHIDValueRef value) {
  if (result != kIOReturnSuccess) {
    return;
  }

  OSXGoliathController *controller = (__bridge OSXGoliathController *)context;

  IOHIDElementRef element = IOHIDValueGetElement(value);
  uint32 usagePage = IOHIDElementGetUsagePage(element);
  uint32 usage = IOHIDElementGetUsage(element);

  if (usagePage == kHIDPage_Button) {
    BOOL buttonState = (BOOL)IOHIDValueGetIntegerValue(value);
    if (usage == controller->_buttonAUsageID) {
      controller->_buttonAState = buttonState;
    }
    if (usage == controller->_buttonBUsageID) {
      controller->_buttonBState = buttonState;
    }
    if (usage == controller->_buttonXUsageID) {
      controller->_buttonXState = buttonState;
    }
    if (usage == controller->_buttonYUsageID) {
      controller->_buttonYState = buttonState;
    }
    if (usage == controller->_lShoulderUsageID) {
      controller->_buttonLeftShoulderState = buttonState;
    }
    if (usage == controller->_rShoulderUsageID) {
      controller->_buttonRightShoulderState = buttonState;
    }
  } else if (usagePage == kHIDPage_GenericDesktop &&
             usage == kHIDUsage_GD_Hatswitch) {
    int dpadState = (int)IOHIDValueGetIntegerValue(value);
    NSInteger dpadX = 0;
    NSInteger dpadY = 0;
    switch (dpadState) {
    case 0:
      dpadX = 0;
      dpadY = 1;
      break;
    case 1:
      dpadX = 1;
      dpadY = 1;
      break;
    case 2:
      dpadX = 1;
      dpadY = 0;
      break;
    case 3:
      dpadX = 1;
      dpadY = -1;
      break;
    case 4:
      dpadX = 0;
      dpadY = -1;
      break;
    case 5:
      dpadX = -1;
      dpadY = -1;
      break;
    case 6:
      dpadX = -1;
      dpadY = 0;
      break;
    case 7:
      dpadX = -1;
      dpadY = 1;
      break;
    default:
      dpadX = 0;
      dpadY = 0;
      break;
    }

    controller->_dpadX = dpadX;
    controller->_dpadY = dpadY;
  }
}

@end

const int samplesPerSecond = 48000;

global_variable MacOSSoundOutput soundOutput = {};

OSStatus circularBufferRenderCallback(void *inRefCon,
                                      AudioUnitRenderActionFlags *ioActionFlags,
                                      const AudioTimeStamp *inTimeStamp,
                                      uint32 inBusNumber, uint32 inNumberFrames,
                                      AudioBufferList *ioData) {

  int length = inNumberFrames * soundOutput.bytesPerSample;
  uint32 region1Size = length;
  uint32 region2Size = 0;

  if (soundOutput.playCursor + length > soundOutput.bufferSize) {
    region1Size = soundOutput.bufferSize - soundOutput.playCursor;
    region2Size = length - region1Size;
  }
  uint8 *channel = (uint8 *)ioData->mBuffers[0].mData;

  memcpy(channel, (uint8 *)soundOutput.data + soundOutput.playCursor,
         region1Size);
  memcpy(&channel[region1Size], soundOutput.data, region2Size);

  soundOutput.playCursor =
      (soundOutput.playCursor + length) % soundOutput.bufferSize;
  /* soundOutput.writeCursor = (soundOutput.playCursor +
   * 2048)%soundOutput.bufferSize; */
  return noErr;
}

/* OSStatus squareWaveRenderCallback(void *inRefCon, */
/*                                   AudioUnitRenderActionFlags *ioActionFlags,
 */
/*                                   const AudioTimeStamp *inTimeStamp, */
/*                                   uint32 inBusNumber, uint32 inNumberFrames,
 */
/*                                   AudioBufferList *ioData) { */
/* #pragma unused(ioActionFlags) */
/* #pragma unused(inTimeStamp) */
/* #pragma unused(inBusNumber) */
/* #pragma unused(inRefCon) */
/**/
/*   int16 *channel = (int16 *)ioData->mBuffers[0].mData; */
/**/
/*   uint32 frequency = 256; */
/*   uint32 period = samplesPerSecond / frequency; */
/*   uint32 halfPeriod = period / 2; */
/*   local_persist uint32 runningSampleIndex = 0; */
/*   for (uint32 i = 0; i < inNumberFrames; i++) { */
/*     if ((runningSampleIndex % period) > halfPeriod) { */
/*       *channel++ = 5000; */
/*       *channel++ = 5000; */
/*     } else { */
/*       *channel++ = -5000; */
/*       *channel++ = -5000; */
/*     } */
/*     runningSampleIndex++; */
/*   } */
/*   return noErr; */
/* } */
/**/

global_variable AudioComponentInstance audioUnit;
internal void macOSInitSound() {

  soundOutput.samplesPerSecond = 48000;
  int audioFrameSize = sizeof(int16) * 2;
  int numberOfSeconds = 2;
  soundOutput.bytesPerSample = audioFrameSize;
  soundOutput.bufferSize =
      soundOutput.samplesPerSecond * audioFrameSize * numberOfSeconds;
  soundOutput.data = malloc(soundOutput.bufferSize);
  soundOutput.playCursor = soundOutput.writeCursor = 0;

  AudioComponentDescription acd;
  acd.componentType = kAudioUnitType_Output;
  acd.componentSubType = kAudioUnitSubType_DefaultOutput;
  acd.componentManufacturer = kAudioUnitManufacturer_Apple;
  acd.componentFlags = 0;
  acd.componentFlagsMask = 0;

  AudioComponent outputComponent = AudioComponentFindNext(NULL, &acd);
  OSStatus status = AudioComponentInstanceNew(outputComponent, &audioUnit);

  if (status != noErr) {
    NSLog(@"There was an error setting up sound");
    return;
  }

  AudioStreamBasicDescription audioDescriptor;
  audioDescriptor.mSampleRate = soundOutput.samplesPerSecond;
  audioDescriptor.mFormatID = kAudioFormatLinearPCM;
  audioDescriptor.mFormatFlags =
      kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;

  int framesPerPacket = 1;
  int bytesPerFrame = sizeof(int16) * 2;
  audioDescriptor.mFramesPerPacket = framesPerPacket;
  audioDescriptor.mChannelsPerFrame = 2;
  audioDescriptor.mBitsPerChannel = sizeof(int16) * 8;
  audioDescriptor.mBytesPerFrame = bytesPerFrame;
  audioDescriptor.mBytesPerPacket = framesPerPacket * bytesPerFrame;

  status = AudioUnitSetProperty(audioUnit, kAudioUnitProperty_StreamFormat,
                                kAudioUnitScope_Input, 0, &audioDescriptor,
                                sizeof(audioDescriptor));
  if (status != noErr) {
    NSLog(@"There was an error setting up the audio unit");
    return;
  }

  AURenderCallbackStruct renderCallback;
  renderCallback.inputProc = circularBufferRenderCallback;

  status = AudioUnitSetProperty(audioUnit, kAudioUnitProperty_SetRenderCallback,
                                kAudioUnitScope_Global, 0, &renderCallback,
                                sizeof(renderCallback));

  if (status != noErr) {
    NSLog(@"There was an error setting up the audio unit");
    return;
  }

  AudioUnitInitialize(audioUnit);
  AudioOutputUnitStart(audioUnit);
}

internal void updateSoundBuffer() {
  int sampleCount = 1600;

  uint32 frequency = 256;
  uint32 period = soundOutput.samplesPerSecond / frequency;
  uint32 halfPeriod = period / 2;
  local_persist uint32 runningSampleIndex = 0;

  int byteToLock = (runningSampleIndex * soundOutput.bytesPerSample) %
                   soundOutput.bufferSize;
  int bytesToWrite;

  if (byteToLock == soundOutput.playCursor) {
    bytesToWrite = soundOutput.bufferSize;
  } else if (byteToLock > soundOutput.playCursor) {
    bytesToWrite = (soundOutput.bufferSize - byteToLock);
    bytesToWrite += soundOutput.playCursor;
  } else {
    bytesToWrite = soundOutput.playCursor - byteToLock;
  }

  void *region1 = (uint8 *)soundOutput.data + byteToLock;
  int region1Size = bytesToWrite;

  if (region1Size + byteToLock > soundOutput.bufferSize) {
    region1Size = soundOutput.bufferSize - byteToLock;
  }

  void *region2 = soundOutput.data;
  int region2Size = bytesToWrite - region1Size;

  int region1SampleCount = region1Size / soundOutput.bytesPerSample;
  int16 *sampleOut = (int16 *)region1;

  for (int sampleIndex = 0; sampleIndex < region1SampleCount; ++sampleIndex) {
    int16 sampleValue =
        ((runningSampleIndex++ / halfPeriod) % 2) ? 5000 : -5000;
    *sampleOut++ = sampleValue;
    *sampleOut++ = sampleValue;
  }

  int region2SampleCount = region2Size / soundOutput.bytesPerSample;
  sampleOut = (int16 *)region2;
  for (int sampleIndex = 0; sampleIndex < region2SampleCount; ++sampleIndex) {
    int16 sampleValue =
        ((runningSampleIndex++ / halfPeriod) % 2) ? 5000 : -5000;
    *sampleOut++ = sampleValue;
    *sampleOut++ = sampleValue;
  }
}

int main(int argc, const char *argv[]) {

  GoliathMainWindowDelegate *mainWindowDelegate =
      [[GoliathMainWindowDelegate alloc] init];
  NSRect screenRect = [[NSScreen mainScreen] frame];
  NSRect initialFrame =
      NSMakeRect((screenRect.size.width - GlobalRenderWidth) * 0.5,
                 (screenRect.size.height - GlobalRenderHeight) * 0.5,
                 GlobalRenderWidth, GlobalRenderHeight);

  NSWindow *window = [[NSWindow alloc]
      initWithContentRect:initialFrame
                styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                          NSWindowStyleMaskMiniaturizable |
                          NSWindowStyleMaskResizable
                  backing:NSBackingStoreBuffered
                    defer:NO];

  [window setBackgroundColor:NSColor.redColor];
  [window setTitle:@"Goliath Game Engine"];
  [window makeKeyAndOrderFront:nil];
  [window setDelegate:mainWindowDelegate];
  window.contentView.wantsLayer = YES;

  macOSRefreshBuffer(window, buffer);
  macOSInitGameControllers();
  macOSInitSound();

  while (Running) {
    renderWeirdGradient(buffer);
    macOSRedrawBuffer(window, buffer);

    OSXGoliathController *controller = keyboardController;

    if (controller != nil) {

      if (controller.buttonAState == true) {
        offsetX++;
      }

      if (controller.buttonLeftShoulderState == true) {
        offsetX--;
      }

      if (controller.buttonRightShoulderState == true) {
        offsetX++;
      }

      if (controller.dpadX == 1) {
        offsetX++;
      }

      if (controller.dpadX == -1) {
        offsetX--;
      }

      if (controller.dpadY == 1) {
        offsetY++;
      }

      if (controller.dpadY == -1) {
        offsetY--;
      }
    }
    NSEvent *event;

    do {
      event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                 untilDate:nil
                                    inMode:NSDefaultRunLoopMode
                                   dequeue:YES];

      if (event != nil && controller == keyboardController &&
              (event.type == NSEventTypeKeyDown) ||
          (event.type == NSEventTypeKeyUp)) {
        updateKeyboardControllerWith(event);
      }
      switch ([event type]) {
      default:
        [NSApp sendEvent:event];
      }
    } while (event != nil);
  }

  printf("Goliath finished running");
}
