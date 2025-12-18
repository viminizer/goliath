#include "goliath_types.h"
#include <cstdlib>

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

  while (Running) {
    renderWeirdGradient(buffer);
    macOSRedrawBuffer(window, buffer);
    offsetX++;
    NSEvent *Event;

    do {
      Event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                 untilDate:nil
                                    inMode:NSDefaultRunLoopMode
                                   dequeue:YES];
      switch ([Event type]) {
      default:
        [NSApp sendEvent:Event];
      }
    } while (Event != nil);
  }

  printf("Goliath finished running");
}
