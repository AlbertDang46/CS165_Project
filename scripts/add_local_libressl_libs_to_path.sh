#!/bin/bash

# NOTE: Always start this script from the project root directory

PROJECT_DIR=$(pwd)
echo "Starting at directory: $PROJECT_DIR"

EXTERN_DIR=$PROJECT_DIR/extern
BUILD_DIR=$PROJECT_DIR/build
CMAKE_DIR=$EXTERN_DIR/cmake
LIBRESSL_DIR=$EXTERN_DIR/libressl
LIBRESSL_INSTALL_DIR=$EXTERN_DIR/libressl_install

LD_LIBRARY_PATH="$LIBRESSL_INSTALL_DIR/lib/":$LD_LIBRARY_PATH
export LD_LIBRARY_PATH
