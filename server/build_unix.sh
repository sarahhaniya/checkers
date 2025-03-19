#!/bin/bash

echo "Building Checkers Game..."

# Create a build directory if it doesn't exist
mkdir -p build
cd build

# Run CMake and build
echo "Running CMake..."
cmake ..

echo "Building project..."
make

echo ""
echo "Build complete! Executables can be found in the build directory."
echo "- checkers_server: Multiplayer server"
echo "- checkers: Original single-player game"
echo ""

cd ..