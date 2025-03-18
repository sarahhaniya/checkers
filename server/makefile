# server/makefile
CXX = g++
CXXFLAGS = -std=c++11 -Wall -pthread
INCLUDES = -I./include -I./GameLogic

# Source files
SERVER_SRC = $(wildcard src/*.cpp)
GAMELOGIC_SRC = $(wildcard GameLogic/*.cpp)
SOURCES = main.cpp $(SERVER_SRC)

# Object files
OBJECTS = $(SOURCES:.cpp=.o)
GAMELOGIC_OBJ = $(filter-out GameLogic/main.o, $(GAMELOGIC_SRC:.cpp=.o))

# Target executable
TARGET = checkers_server

# Rules
all: $(TARGET)

$(TARGET): $(OBJECTS) $(GAMELOGIC_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<
	
# ThreadPool test
test_threadpool: test_threadpool.o src/ThreadPool.o
	$(CXX) $(CXXFLAGS) -o $@ $^

test_threadpool.o: test_threadpool.cpp include/ThreadPool.h
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<

src/ThreadPool.o: src/ThreadPool.cpp include/ThreadPool.h
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<

test: test_threadpool
	./test_threadpool

clean:
	rm -f $(OBJECTS) $(TARGET) test_threadpool test_threadpool.o src/*.o

.PHONY: all clean test


# Server test
test_server: test_server.o src/Server.o src/ThreadPool.o src/Session.o src/Utilities.o GameLogic/Board.o GameLogic/Move.o GameLogic/Piece.o GameLogic/HumanPlayer.o
	$(CXX) $(CXXFLAGS) -o $@ $^

src/Session.o: src/Session.cpp include/Session.h
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<

	# Add to your makefile
src/Utilities.o: src/Utilities.cpp include/Utilities.h
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<

