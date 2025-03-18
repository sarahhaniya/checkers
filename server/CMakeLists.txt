cd ../server
cat > CMakeLists.txt << 'EOL'
cmake_minimum_required(VERSION 3.10)
project(CheckersServer)

set(CMAKE_CXX_STANDARD 17)

# Add source files
file(GLOB_RECURSE SOURCES "src/*.cpp")

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE src)

# Add any required libraries here
# For example:
# find_package(Boost REQUIRED COMPONENTS system thread)
# target_link_libraries(${PROJECT_NAME} Boost::system Boost::thread)
EOL