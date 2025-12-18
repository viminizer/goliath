echo Building Goliath Game Engine

OSX_LD_FLAGS="-framework AppKit"

mkdir ../build
pushd ../build
clang -g $OSX_LD_FLAGS -o goliath ../code/osx_main.mm
popd
