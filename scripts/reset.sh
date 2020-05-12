#!/bin/bash

# NOTE: Always start this script from the project root directory

PROJECT_DIR=$(pwd)
echo "Starting at directory: $PROJECT_DIR"

EXTERN_DIR=$PROJECT_DIR/extern
BUILD_DIR=$PROJECT_DIR/build
CMAKE_DIR=$EXTERN_DIR/cmake
LIBRESSL_DIR=$EXTERN_DIR/libressl

rm -r $BUILD_DIR
rm -r $CMAKE_DIR
rm -r $LIBRESSL_DIR
