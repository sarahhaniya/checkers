## Prerequisites

### Windows
- C++ compiler (Visual Studio or MinGW)
- CMake 3.10 or higher
- Git (for cloning the repository)

### macOS
- Xcode Command Line Tools (`xcode-select --install`)
- CMake 3.10 or higher (`brew install cmake`)
- Git (for cloning the repository)

### Linux
- G++ compiler (`sudo apt-get install build-essential`)
- CMake 3.10 or higher (`sudo apt-get install cmake`)
- Git (for cloning the repository)

## Building the Game

### Windows

**Using Visual Studio:**

1. Open a Command Prompt or PowerShell window
2. Navigate to the repository directory
3. Create a build directory and navigate into it:
   ```
   cd server
   mkdir build
   cd build
   ```
4. Generate Visual Studio project files:
   ```
   cmake -G "Visual Studio 16 2019" ..
   ```
   Note: Replace "Visual Studio 16 2019" with your Visual Studio version if different

5. Build the project:
   ```
   cmake --build . --config Release
   ```

6. The executables will be in the `Release` folder inside the build directory

**Using MinGW:**

1. Open a Command Prompt or PowerShell window
2. Navigate to the repository directory
3. Create a build directory and navigate into it:
   ```
   cd server
   mkdir build
   cd build
   ```
4. Generate MinGW makefiles:
   ```
   cmake -G "MinGW Makefiles" ..
   ```
5. Build the project:
   ```
   mingw32-make
   ```

### macOS

1. Open Terminal
2. Navigate to the repository directory
3. Create a build directory and navigate into it:
   ```
   cd server
   mkdir build
   cd build
   ```
4. Generate makefiles and build:
   ```
   cmake ..
   make
   ```

## Running the Game

### Starting the Server

1. Navigate to the folder containing the built executables:
   - Windows (Visual Studio): `server\build\Release`
   - Windows (MinGW)/macOS/Linux: `server/build`

2. Run the server:
   - Windows: `checkers_server.exe`
   - macOS/Linux: `./checkers_server`

3. The server will display the port it's running on (default is 8080, but it will try subsequent ports if 8080 is unavailable)

### Connecting to the Server

You can connect to the server using `netcat` on macOS/Linux or `telnet` on Windows:

#### macOS/Linux
```
nc localhost 8080
```

#### Windows
```
telnet localhost 8080
```

If you're connecting to a server on another computer, replace `localhost` with the server's IP address.

## Playing the Game

1. After connecting, log in with a username:
   ```
   LOGIN username
   ```

2. Player 1 creates a game:
   ```
   CREATE
   ```
   (Note the game ID that appears)

3. Player 2 logs in and joins the game:
   ```
   LOGIN player2
   JOIN gameID
   ```

4. Make moves using the coordinates shown on the board:
   ```
   MOVE fromX fromY toX toY
   ```
   For example: `MOVE 2 2 3 3`

5. Other commands:
   ```
   STATE     - Show the current board state
   HELP      - Show available commands
   ```

## Game Rules

- White pieces are shown as `W` (or `WK` for kings)
- Black pieces are shown as `B` (or `BK` for kings)
- Players take turns moving pieces
- If a jump is available, you must take it
- Kings are created when a piece reaches the opposite end of the board
- The winner is the player who captures all of the opponent's pieces

## Troubleshooting

### Common Issues

1. **Can't connect to the server**
   - Make sure the server is running
   - Check the port number shown in the server output
   - Verify any firewall settings that might block the connection

2. **Server won't start**
   - If you get a "port in use" error, the server will try several alternate ports automatically
   - If all ports fail, try stopping other applications that might be using the ports

3. **Build errors**
   - Make sure you have all the prerequisites installed
   - Check error messages for specific missing dependencies

### Platform-Specific Issues

#### Windows
- If you get socket errors, make sure your version of Windows supports Winsock 2
- If using Visual Studio, ensure you are building for the correct architecture (x86 or x64)

#### macOS
- If you get permission errors, try running the command with `sudo`
- On newer macOS versions, you might need to allow the application in Security & Privacy settings
