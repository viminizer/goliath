#!/bin/bash
set -e

APP_NAME="goliath.app"
APP_PATH="build/$APP_NAME"

if [ ! -d "$APP_PATH" ]; then
  echo "Error: $APP_PATH not found. Build the app first."
  exit 1
fi

# Ensure executable permission
chmod +x "$APP_PATH/Contents/MacOS/goliath"

# Remove quarantine flag if present (dev builds)
xattr -dr com.apple.quarantine "$APP_PATH" 2>/dev/null || true

# Launch the app
open "$APP_PATH"
