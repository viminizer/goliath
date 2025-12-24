echo "Building Goliath Game Engine"

OSX_LD_FLAGS="-framework AppKit \
              -framework IOKit \
              -framework AudioToolbox"

GOLIATH_RESOURCES_PATH="resources"

rm -rf build
mkdir build
pushd build

clang++ -g \
  ../src/goliath.cpp \
  ../src/osx_main.mm \
  $OSX_LD_FLAGS \
  -o goliath

rm -rf goliath.app
mkdir -p goliath.app/Contents/MacOS

cp goliath goliath.app/Contents/MacOS/goliath
cp ../$GOLIATH_RESOURCES_PATH/Info.plist goliath.app/Contents/Info.plist

popd
