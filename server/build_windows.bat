@echo off
echo Building Checkers Game for Windows...

REM Create a build directory if it doesn't exist
if not exist build mkdir build
cd build

REM Run CMake to generate Visual Studio project files
echo Running CMake...
cmake -G "Visual Studio 16 2019" ..

echo.
echo CMake configuration done. You can now:
echo 1. Open the generated .sln file in Visual Studio and build from there
echo 2. Build from command line

REM Prompt user for option
set /p option="Would you like to build now from command line? (y/n): "
if /i "%option%"=="y" (
    echo Building project...
    cmake --build . --config Release
    echo.
    echo Build complete! Executables can be found in build\Release directory.
)

cd ..