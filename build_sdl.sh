#!/bin/bash

set -e

echo "Running Goliath Game Engine (SDL)"

rm -rf sdl_build
mkdir sdl_build
pushd sdl_build

CFLAGS="$(sdl2-config --cflags)"
LDFLAGS="$(sdl2-config --libs)"

clang++ \
  $CFLAGS \
  ../src/sdl_goliath.cpp \
  $LDFLAGS \
  -o sdl_goliath

popd
