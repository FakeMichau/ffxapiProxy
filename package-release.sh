#!/bin/bash

set -e

shopt -s extglob

if [ -z "$1" ] || [ -z "$2" ]; then
  echo "Usage: $0 version destdir"
  exit 1
fi

VERSION="$1"
SRC_DIR=$(dirname "$(readlink -f "$0")")
BUILD_DIR=$(realpath "$2")"/ffxapiProxy-$VERSION"

if [ -e "$BUILD_DIR" ]; then
  echo "Build directory $BUILD_DIR already exists"
  exit 1
fi

meson setup                                \
  --cross-file "$SRC_DIR/build-win.txt" \
  --buildtype "release"                    \
  --prefix "$BUILD_DIR"                    \
  --strip                                  \
  --bindir "bin"                           \
  --libdir "bin"                           \
  "$BUILD_DIR/build"

cd "$BUILD_DIR/build"
ninja install

