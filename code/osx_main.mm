#include <AppKit/AppKit.h>
#include <Foundation/Foundation.h>
#include <cstdint>
#include <stdio.h>

#define internal static
#define local_persist static
#define global_variable static

global_variable float GlobalRenderWidth = 1024;
global_variable float GlobalRenderHeight = 768;
global_variable bool Running = true;
global_variable uint8_t *buffer;

@interface GoliathMainWindowDelegate : NSObject <NSWindowDelegate>
@end

@implementation GoliathMainWindowDelegate

- (void)windowWillClose:(id)sender {
  Running = false;
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

  int bitmapWidth = window.contentView.bounds.size.width;
  int bitmapHeight = window.contentView.bounds.size.height;
  int bytesPerPixel = 4;
  int pitch = bitmapWidth * bytesPerPixel;
  buffer = (uint8_t *)malloc(pitch * bitmapHeight);

  while (Running) {

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
