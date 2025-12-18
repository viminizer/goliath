#include <AppKit/AppKit.h>
#include <stdio.h>

#define internal static
#define local_persist static
#define global_variable static

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

global_variable float GlobalRenderWidth = 1024;
global_variable float GlobalRenderHeight = 768;

global_variable bool Running = true;
global_variable uint8 *buffer;
global_variable int bitmapWidth;
global_variable int bitmapHeight;
global_variable int bytesPerPixel = 4;
global_variable int pitch;

global_variable int offsetX = 0;

void macOSRefreshBuffer(NSWindow *window) {
  if (buffer) {
    free(buffer)
  }
  bitmapWidth = window.contentView.bounds.size.width;
  bitmapHeight = window.contentView.bounds.size.height;
  pitch = bitmapWidth * bytesPerPixel;
  buffer = (uint8 *)malloc(pitch * bitmapHeight);
}

void renderWeirdGradient() {
  int width = bitmapWidth;
  int height = bitmapHeight;
  uint8 *row = (uint8 *)buffer;

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
    row += pitch;
  }
}

void macOSRedrawBuffer(NSWindow *window) {
  @autoreleasepool {
    NSBitmapImageRep *rep = [[[NSBitmapImageRep alloc]
        initWithBitmapDataPlanes:&buffer
                      pixelsWide:bitmapWidth
                      pixelsHigh:bitmapHeight
                   bitsPerSample:8
                 samplesPerPixel:4
                        hasAlpha:YES
                        isPlanar:NO
                  colorSpaceName:NSDeviceRGBColorSpace
                     bytesPerRow:pitch
                    bitsPerPixel:bytesPerPixel * 8] autorelease];
    NSSize imageSize = NSMakeSize(bitmapWidth, bitmapHeight);
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
  macOSRefreshBuffer(window);
  renderWeirdGradient();
  macOSRedrawBuffer(window);
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

  macOSRefreshBuffer(window);

  while (Running) {
    renderWeirdGradient();
    macOSRedrawBuffer(window);
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
