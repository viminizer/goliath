#!/bin/bash

set -e

echo "=== Goliath Game Engine ==="
echo ""
echo "Building complete application..."
echo ""

# Clean and create build directory
rm -rf build
mkdir build
pushd build

CFLAGS="$(sdl2-config --cflags)"
LDFLAGS="$(sdl2-config --libs)"

# Step 1: Build game library
echo "Building game library..."
clang++ \
  -DGOLIATH_INTERNAL=1 \
  -DGOLIATH_SLOW=1 \
  -dynamiclib \
  -undefined dynamic_lookup \
  -install_name @rpath/libgoliath.dylib \
  -o libgoliath.dylib \
  ../src/goliath.cpp

# Step 2: Build platform layer
echo "Building platform layer..."
clang++ \
  -DGOLIATH_INTERNAL=1 \
  -DGOLIATH_SLOW=1 \
  $CFLAGS \
  ../src/sdl_goliath.cpp \
  $LDFLAGS \
  -L. \
  -lgoliath \
  -Wl,-rpath,@executable_path \
  -o goliath

popd

echo ""
echo "Build complete!"
echo "  Library: build/libgoliath.dylib"
echo "  Executable: build/goliath"
echo ""
echo "=== Running Goliath ==="
echo ""

# Run the game
./build/goliath
