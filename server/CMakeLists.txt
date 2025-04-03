cmake_minimum_required(VERSION 3.10)
project(checkers_game)

# Specify C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define ASIO_STANDALONE to use standalone ASIO instead of Boost
add_definitions(-DASIO_STANDALONE)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/GameLogic
    ${CMAKE_CURRENT_SOURCE_DIR}/websocketpp
    ${CMAKE_CURRENT_SOURCE_DIR}/asio/asio/include  # Path to standalone ASIO
)

# Gather source files
file(GLOB SERVER_SRC "src/*.cpp")
file(GLOB GAMELOGIC_SRC "GameLogic/*.cpp")

# Filter out main.cpp from GameLogic if it exists to avoid duplicate main functions
list(FILTER GAMELOGIC_SRC EXCLUDE REGEX ".*main\\.cpp$")

# Server executable (using test_server.cpp as entry point)
add_executable(checkers_server 
    test_server.cpp
    ${SERVER_SRC}
    ${GAMELOGIC_SRC}
)

# Find and link thread library
find_package(Threads REQUIRED)
target_link_libraries(checkers_server PRIVATE Threads::Threads)

# Windows-specific settings
if(WIN32)
    target_link_libraries(checkers_server PRIVATE wsock32 ws2_32)
endif()

# Add a separate target for the original single-player checkers game
add_executable(checkers
    GameLogic/main.cpp
    GameLogic/Board.cpp
    GameLogic/Move.cpp
    GameLogic/Piece.cpp
    GameLogic/HumanPlayer.cpp
)

# Installation rules
install(TARGETS checkers_server checkers
        RUNTIME DESTINATION bin)