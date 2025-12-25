#!/bin/bash

set -e

PROJECT_ROOT="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$PROJECT_ROOT/sdl_build"
SRC_FILE="$PROJECT_ROOT/src/sdl_goliath.cpp"
OUT_FILE="$BUILD_DIR/sdl_goliath"

mkdir -p "$BUILD_DIR"

CFLAGS="$(sdl2-config --cflags)"
LDFLAGS="$(sdl2-config --libs)"

cat >"$PROJECT_ROOT/compile_commands.json" <<EOF
[
  {
    "directory": "$PROJECT_ROOT",
    "command": "clang++ $CFLAGS -g -O0 -c $SRC_FILE",
    "file": "$SRC_FILE"
  }
]
EOF

echo "compile_commands.json generated at:"
echo "$PROJECT_ROOT/compile_commands.json"
