#!/bin/bash
set -e

echo "Building game library (libgoliath.dylib)..."

# Create build directory if it doesn't exist
mkdir -p build
pushd build

# Compile platform-independent code as a dynamic library
clang++ \
  -DGOLIATH_INTERNAL=1 \
  -DGOLIATH_SLOW=1 \
  -dynamiclib \
  -undefined dynamic_lookup \
  -install_name @rpath/libgoliath.dylib \
  -o libgoliath.dylib \
  ../src/goliath.cpp

echo "Game library built: build/libgoliath.dylib"

popd
