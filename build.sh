#!/bin/sh
# build.sh - Automated build script for calc

set -e

BUILD_DIR="build"
PATH="/opt/trinity/bin:$PATH"
export PATH

# Handle cleaning
if [ "$1" = "clean" ]; then
    echo "=== Cleaning build artifacts ==="
    if [ -d "$BUILD_DIR" ]; then
        rm -rf "$BUILD_DIR"
    fi
    echo "Clean completed."
    exit 0
fi

echo "=== Preparing Build Directory ==="
mkdir -p "$BUILD_DIR"

cd "$BUILD_DIR"

# Determine compression method
COMPRESS_METHOD="zx0"
if [ "$1" = "dev" ]; then
    COMPRESS_METHOD="lz4"
    echo "=== DEV Mode: Using fast LZ4 compression ==="
elif [ "$1" = "zlib" ]; then
    COMPRESS_METHOD="zlib"
    echo "=== ZLIB Mode: Using Zlib compression ==="
else
    echo "=== RELEASE Mode: Using maximum ZX0 compression ==="
fi

# Setup Cmake arguments for Clang LTO compilation
CMAKE_ARGS="-DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang -DCOMPRESS=$COMPRESS_METHOD"

if [ -f "/usr/bin/llvm-ar-19" ]; then
    CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_AR=/usr/bin/llvm-ar-19"
elif [ -f "/usr/lib/llvm-19/bin/llvm-ar" ]; then
    CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_AR=/usr/lib/llvm-19/bin/llvm-ar"
fi

if [ -f "/usr/bin/llvm-ranlib-19" ]; then
    CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_RANLIB=/usr/bin/llvm-ranlib-19"
elif [ -f "/usr/lib/llvm-19/bin/llvm-ranlib" ]; then
    CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_RANLIB=/usr/lib/llvm-19/bin/llvm-ranlib"
fi

echo "=== Running CMake ==="
# We force Clang as the compiler for optimized release builds
cmake $CMAKE_ARGS ..

echo "=== Compiling calc executable ==="
# Building via make (which automatically runs the python script to regenerate assets if they changed)
make -j$(nproc)

echo "=== Build Successful ==="
echo "Executable located at: $BUILD_DIR/calc"
echo "You can run it using: ./$BUILD_DIR/calc"
