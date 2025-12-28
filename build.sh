#!/bin/bash

set -e

echo "Running Goliath Game Engine (SDL)"

rm -rf build
mkdir build
pushd build

CFLAGS="$(sdl2-config --cflags)"
LDFLAGS="$(sdl2-config --libs)"

clang++ \
  -DGOLIATH_INTERNAL=1 \
  -DGOLIATH_SLOW=1 \
  $CFLAGS \
  ../src/goliath.cpp \
  ../src/sdl_goliath.cpp \
  $LDFLAGS \
  -o goliath

popd
