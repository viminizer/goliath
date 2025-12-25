#!/bin/bash

set -e

echo "Running Goliath Game Engine (SDL DEBUG)"

BUILD_DIR="build_debug"

rm -rf "$BUILD_DIR"
mkdir "$BUILD_DIR"
pushd "$BUILD_DIR" >/dev/null

CFLAGS="$(sdl2-config --cflags)"
LDFLAGS="$(sdl2-config --libs)"

clang++ \
  -g \
  -O0 \
  -DDEBUG=1 \
  -DGOLIATH_INTERNAL=1 \
  -DGOLIATH_SLOW=1 \
  -Wall -Wextra \
  $CFLAGS \
  ../src/goliath.cpp \
  ../src/sdl_goliath.cpp \
  $LDFLAGS \
  -o goliath_debug

# Run under debugger if requested
if [[ "$1" == "lldb" ]]; then
  lldb ./goliath_debug
elif [[ "$1" == "gdb" ]]; then
  gdb ./goliath_debug
else
  lldb ./goliath_debug
fi

popd >/dev/null
