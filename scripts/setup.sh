#!/bin/bash

# Always start from the main directory
PROJECT_DIR=$(pwd)
PROJECT_BUILD_DIR=$PROJECT_DIR/build
echo "Starting Setup... $PROJECT_DIR"

NUM_PARALLEL_BUILDS=12

EXTERN_DIR=$PROJECT_DIR/extern
CMAKE_DIR=$EXTERN_DIR/cmake
CMAKE_BIN_DIR=$CMAKE_DIR/bin
LIBRESSL_DIR=$EXTERN_DIR/libressl
LIBRESSL_BUILD_DIR=$LIBRESSL_DIR/build
LIBRESSL_INSTALL_DIR=$EXTERN_DIR/libressl_install

# 1. Extract the archived third party libraries
mkdir $LIBRESSL_DIR
mkdir $CMAKE_DIR
tar -xvzf $EXTERN_DIR/libressl-3.1.1.tar.gz -C $LIBRESSL_DIR --strip-components=1
tar -xvzf $EXTERN_DIR/cmake-3.17.2-Linux-x86_64.tar.gz -C $CMAKE_DIR --strip-components=1

# 2. Build libressl
mkdir $LIBRESSL_BUILD_DIR
mkdir $LIBRESSL_INSTALL_DIR
cd $LIBRESSL_BUILD_DIR
$CMAKE_BIN_DIR/cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX:PATH="$LIBRESSL_INSTALL_DIR" -DBUILD_SHARED_LIBS=ON $LIBRESSL_DIR
make -j$NUM_PARALLEL_BUILDS
echo "LibreSSL built!"

make install
echo "LibreSSL installed to: $LIBRESSL_INSTALL_DIR"

echo "Setting up environment variables"
LD_LIBRARY_PATH="$LIBRESSL_INSTALL_DIR/lib/":$LD_LIBRARY_PATH
export LD_LIBRARY_PATH

# 3. Build code
mkdir $PROJECT_BUILD_DIR && cd $PROJECT_BUILD_DIR
$CMAKE_BIN_DIR/cmake -DCMAKE_BUILD_TYPE=Release -DLIBRESSL_ROOT_DIR="$EXTERN_DIR/libressl_install" $PROJECT_DIR
make -j$NUM_PARALLEL_BUILDS


