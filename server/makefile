# makefile

CXX = g++
CXXFLAGS = -std=c++17 -Wall -pthread

# Platform detection
ifeq ($(OS),Windows_NT)
    EXE_EXT = .exe
    PLATFORM_LIBS = -lws2_32 -lmswsock
    RM = del /Q /F
    NULLDEV = nul
else
    EXE_EXT =
    PLATFORM_LIBS =
    RM = rm -f
    NULLDEV = /dev/null
endif

# Includes
INCLUDES = -I./include -I./GameLogic -I./asio

# Source files
SERVER_SRC = $(wildcard src/*.cpp)
GAMELOGIC_SRC = $(wildcard GameLogic/*.cpp)
SOURCES = main.cpp $(SERVER_SRC)

# Object files
OBJECTS = $(SOURCES:.cpp=.o)
GAMELOGIC_OBJ = $(filter-out GameLogic/main.o, $(GAMELOGIC_SRC:.cpp=.o))

# Executable name
TARGET = checkers_server$(EXE_EXT)

# Default build
all: $(TARGET)

$(TARGET): $(OBJECTS) $(GAMELOGIC_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(PLATFORM_LIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<

# ThreadPool test
test_threadpool: test_threadpool.o src/ThreadPool.o
	$(CXX) $(CXXFLAGS) -o test_threadpool$(EXE_EXT) $^ $(PLATFORM_LIBS)

test_threadpool.o: test_threadpool.cpp include/ThreadPool.h
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<

src/ThreadPool.o: src/ThreadPool.cpp include/ThreadPool.h
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<

# Server test build
test_server$(EXE_EXT): test_server.o src/Server.o src/ThreadPool.o src/Session.o src/Utilities.o src/sqlite3.o src/DatabaseManager.o GameLogic/Board.o GameLogic/Move.o GameLogic/Piece.o GameLogic/HumanPlayer.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(PLATFORM_LIBS)

src/sqlite3.o: src/sqlite3.c
	gcc -c -o $@ $< $(INCLUDES)

src/DatabaseManager.o: src/DatabaseManager.cpp src/DatabaseManager.h
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<
	
test_server.o: test_server.cpp include/Server.h include/ThreadPool.h include/Session.h
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<

src/Session.o: src/Session.cpp include/Session.h
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<

src/Utilities.o: src/Utilities.cpp include/Utilities.h
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<

# Test runner
test: test_threadpool
	./test_threadpool$(EXE_EXT)

# Clean
clean:
	-$(RM) *.o 2> $(NULLDEV)
	-$(RM) src\*.o 2> $(NULLDEV)
	-$(RM) GameLogic\*.o 2> $(NULLDEV)
	-$(RM) $(TARGET) test_server$(EXE_EXT) test_threadpool$(EXE_EXT) 2> $(NULLDEV)

.PHONY: all clean test
