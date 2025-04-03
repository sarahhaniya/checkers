// server/src/Server.cpp
#include "../include/Server.h"
#include "../include/ThreadPool.h"
#include "../include/Session.h"
#include "../include/SocketWrapper.h"

#include <iostream>
#include <cstring>
#include <thread>
#include <chrono>

void killPreviousInstances()
{
#ifdef _WIN32
    // Windows implementation (using taskkill)
    system("taskkill /F /IM test_server.exe /T 2>nul");
    system("taskkill /F /IM checkers_server.exe /T 2>nul");
#else
    // Unix/Linux implementation
    pid_t currentPid = getpid();

    std::string cmd = "ps -ef | grep test_server | grep -v grep | grep -v " +
                      std::to_string(currentPid) +
                      " | awk '{print $2}' | xargs -I {} kill -9 {} 2>/dev/null";

    system(cmd.c_str());
#endif
    // Give a moment for processes to terminate
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

Server::Server(int port, int numThreads)
    : port(port),
      serverSocket(SOCKET_ERROR_VALUE),
      running(false),
      nextSessionId(1)
{
    // Initialize socket library (Windows needs this)
    SocketWrapper::initialize();

    // Create the thread pool
    threadPool = new ThreadPool(numThreads);
}

Server::~Server()
{
    // Clean up resources
    stop();

    // Clean up all game sessions
    for (auto &pair : gameSessions)
    {
        delete pair.second;
    }
 

    // Clean up thread pool
    delete threadPool;

    // Cleanup socket library (Windows needs this)
    SocketWrapper::cleanup();
}

bool Server::start()
{
     // Create socket
     serverSocket = SocketWrapper::createSocket();
     if (serverSocket == SOCKET_ERROR_VALUE)
     {
         std::cerr << "Failed to create socket: " << SocketWrapper::getLastError() << std::endl;
         return false;
     }
 
     // Set socket options
     if (!SocketWrapper::setReuseAddr(serverSocket))
     {
         std::cerr << "Failed to set socket options: " << SocketWrapper::getLastError() << std::endl;
         SocketWrapper::closeSocket(serverSocket);
         return false;
     }
 
     // Try to bind to the initial port, and if that fails, try subsequent ports
     const int MAX_PORT_ATTEMPTS = 10;
     bool bound = false;
 
     for (int attempt = 0; attempt < MAX_PORT_ATTEMPTS; attempt++)
     {
         if (SocketWrapper::bindSocket(serverSocket, port + attempt))
         {
             // Successfully bound to a port
             port = port + attempt; // Update the port to the one we actually bound to
             bound = true;
             break;
         }
 
         std::cerr << "Failed to bind to port " << (port + attempt)
                   << ": " << SocketWrapper::getLastError() << std::endl;
     }
 
     if (!bound)
     {
         std::cerr << "Failed to bind to any port after " << MAX_PORT_ATTEMPTS << " attempts" << std::endl;
         SocketWrapper::closeSocket(serverSocket);
         return false;
     }
 
     // Listen for connections
     if (!SocketWrapper::listenSocket(serverSocket, 10))
     {
         std::cerr << "Failed to listen: " << SocketWrapper::getLastError() << std::endl;
         SocketWrapper::closeSocket(serverSocket);
         return false;
     }
 
     // Mark as running
     running = true;
 
     // Start accept thread
     acceptThread = std::thread(&Server::acceptConnections, this);
 
     // comment out this part of the code up until the return statement to see it work in the terminal
     // then do the make clean process again
     // Initialize WebSocket server 
     wsServer.clear_access_channels(websocketpp::log::alevel::all);
     wsServer.set_access_channels(websocketpp::log::alevel::access_core);
     wsServer.set_access_channels(websocketpp::log::alevel::app);
     
     wsServer.init_asio();
     
     // Set handlers
     wsServer.set_open_handler([this](websocketpp::connection_hdl hdl) {
         this->onWebSocketOpen(hdl);
     });
     
     wsServer.set_close_handler([this](websocketpp::connection_hdl hdl) {
         this->onWebSocketClose(hdl);
     });
     
     wsServer.set_message_handler([this](websocketpp::connection_hdl hdl, message_ptr msg) {
         this->onWebSocketMessage(hdl, msg);
     });
     
     // Listen on WebSocket port (8080)
     try {
         wsServer.listen(8080);
         wsServer.start_accept();
         
         // Start WebSocket server thread
         std::thread wsThread(&WebSocketServer::run, &wsServer);
         wsThread.detach();  // Using detach instead of leaving the thread local
         
         std::cout << "WebSocket server started on port 8080" << std::endl;
     } catch (const websocketpp::exception& e) {
         std::cerr << "WebSocket server error: " << e.what() << std::endl;
         // Continue running the TCP server even if WebSocket fails
     }
 
     std::cout << "Server started on port " << port << std::endl;
     return true;
}

void Server::onWebSocketOpen(websocketpp::connection_hdl hdl) {
    std::cout << "WebSocket connection opened" << std::endl;
    // Store connection handle with placeholder client ID
    wsConnections[hdl] = "Unknown";
}

void Server::onWebSocketClose(websocketpp::connection_hdl hdl) {
    std::cout << "WebSocket connection closed" << std::endl;
    // Remove from connections map
    wsConnections.erase(hdl);
}

void Server::onWebSocketMessage(websocketpp::connection_hdl hdl, message_ptr msg) {
    std::string message = msg->get_payload();
    std::cout << "Received WebSocket message: " << message << std::endl;
    
    // Process commands similar to handleClientConnection
    std::string upperMessage = message;
    for (char &c : upperMessage) {
        c = toupper(c);
    }
    
    std::string response;
    
    try {
        // Parse commands similar to your existing code
        if (upperMessage.find("LOGIN") == 0) {
            // Format: LOGIN username password
            size_t firstSpace = message.find(" ");
            size_t secondSpace = message.find(" ", firstSpace + 1);
            
            if (firstSpace != std::string::npos && secondSpace != std::string::npos) {
                std::string username = message.substr(firstSpace + 1, secondSpace - firstSpace - 1);
                std::string password = message.substr(secondSpace + 1);
                
                // verify password here
                //...
                
                wsConnections[hdl] = username;
                response = "{ \"type\": \"login_success\", \"username\": \"" + username + "\" }";
            } else {
                response = "{ \"type\": \"error\", \"message\": \"Invalid login format\" }";
            }
        } else if (upperMessage.find("REGISTER") == 0) {
            // Format: REGISTER email username password
            // Parse and handle registration
            // ...
            response = "{ \"type\": \"register_success\" }";
        } else if (upperMessage.find("CREATE") == 0) {
            // Check if user is logged in
            std::string clientId = wsConnections[hdl];
            if (clientId != "Unknown") {
                int gameSessionId = createGameSession(clientId);

                   // Get the session and add this client's socket
                   GameSession *session = getGameSession(gameSessionId);
                   if (session)
                   {
                       session->addWebSocketHandle(hdl, &wsServer);
                   }
                
                response = "{ \"type\": \"game_created\", \"gameCode\": \"" + gameCodes[gameSessionId] + "\" }";
            } else {
                response = "{ \"type\": \"error\", \"message\": \"Please login first\" }";
            }
        } else if (upperMessage.find("JOIN") == 0) {
            // Format: JOIN gameId
            size_t pos = message.find(" ");
            if (pos != std::string::npos) {
                std::string clientId = wsConnections[hdl];
                if (clientId != "Unknown") {
                    std::string code = message.substr(pos + 1);
                    int sessionId = 0;
                    for (const auto& [key, value] : gameCodes){
                     if (value == code){
                      sessionId = key;
                     }
                    }
              
                    if (joinGameSession(sessionId, clientId)) {
                        GameSession* session = getGameSession(sessionId);
                        if (session) {
                            // Add WebSocket to notify for game updates
                            session->addWebSocketHandle(hdl, &wsServer);
                            session->broadcastGameState();

                            // Get and send game state
                            std::string boardState = session->getBoardStateJson();
                            response = boardState;
                        }
                    } else {
                        response = "{ \"type\": \"error\", \"message\": \"Failed to join game\" }";
                    }
                } else {
                    response = "{ \"type\": \"error\", \"message\": \"Please login first\" }";
                }
            }
        }
        // Add other commands (MOVE, STATE, etc.)
        
    } catch (const std::exception& e) {
        std::cerr << "Exception processing WebSocket command: " << e.what() << std::endl;
        response = "{ \"type\": \"error\", \"message\": \"Server error processing command\" }";
    }
    
    // Send response
    try {
        wsServer.send(hdl, response, websocketpp::frame::opcode::text);
    } catch (const websocketpp::exception& e) {
        std::cerr << "Failed to send WebSocket response: " << e.what() << std::endl;
    }
}

void Server::stop()
{
    // Stop accepting new connections
    running = false;

    // Close server socket
    if (serverSocket != SOCKET_ERROR_VALUE)
    {
        SocketWrapper::closeSocket(serverSocket);
        serverSocket = SOCKET_ERROR_VALUE;
    }

    // Join accept thread
    if (acceptThread.joinable())
    {
        acceptThread.join();
    }

    // Close all client sockets
    std::lock_guard<std::mutex> lock(sessionsMutex);
    for (auto &pair : gameSessions)
    {
        delete pair.second;
    }
    gameSessions.clear();
  
    gameCodes.clear();

    std::cout << "Server stopped" << std::endl;
}

void Server::acceptConnections()
{
    struct sockaddr_in clientAddress;
    socklen_t clientAddressLength = sizeof(clientAddress);

    while (running)
    {
        // Accept a connection
        socket_t clientSocket = SocketWrapper::acceptConnection(serverSocket, &clientAddress, &clientAddressLength);

        if (clientSocket == SOCKET_ERROR_VALUE)
        {
            // Check if the error is because we're shutting down
            if (!running)
            {
                break;
            }

            std::cerr << "Failed to accept connection: " << SocketWrapper::getLastError() << std::endl;
            continue;
        }

        std::cout << "New connection from "
                  << inet_ntoa(clientAddress.sin_addr) << ":"
                  << ntohs(clientAddress.sin_port) << std::endl;

        // Handle the connection in the thread pool
        threadPool->enqueue([this, clientSocket]()
                            { this->handleClientConnection(clientSocket); });
    }
}

void Server::handleClientConnection(socket_t clientSocket)
{
    const int bufferSize = 1024;
    char buffer[bufferSize];
    std::string clientId = "Unknown"; // Will be set during authentication
    int gameSessionId = -1;           // -1 means not in a game

    try
    {
        // Send welcome message
        std::string welcome = "Welcome to Checkers Server\n";
        SocketWrapper::sendData(clientSocket, welcome.c_str(), welcome.length());

        while (running)
        {
            try
            {
                // Read data from client
                ssize_t bytesRead = SocketWrapper::receiveData(clientSocket, buffer, bufferSize - 1);

                if (bytesRead <= 0)
                {
                    // Client disconnected or error
                    break;
                }

                // Null-terminate the received data
                buffer[bytesRead] = '\0';
                std::string message(buffer);
                std::cout << "Received: " << message << std::endl;

                // Create uppercase version for case-insensitive command matching
                std::string upperMessage = message;
                for (char &c : upperMessage)
                {
                    c = toupper(c);
                }

                // Command processing with error handling
                try
                {
                    // Simple command parsing (you'd replace this with proper protocol handling)
                    if (upperMessage.find("LOGIN") == 0)
                    {
                        // Format: LOGIN username
                        size_t pos = message.find(" ");
                        if (pos != std::string::npos)
                        {
                            clientId = message.substr(pos + 1);
                            std::string response = "Logged in as " + clientId + "\n";
                            SocketWrapper::sendData(clientSocket, response.c_str(), response.length());
                        }
                    }
                    else if (upperMessage.find("CREATE") == 0)
                    {
                        // Create a new game
                        if (clientId != "Unknown")
                        {
                            gameSessionId = createGameSession(clientId);

                            // Get the session and add this client's socket
                            GameSession *session = getGameSession(gameSessionId);
                            if (session)
                            {
                                session->addClientSocket(clientSocket);
                            }

                            std::string response = "Game created with ID: " + std::to_string(gameSessionId) + "\n";
                            SocketWrapper::sendData(clientSocket, response.c_str(), response.length());
                        }
                        else
                        {
                            std::string response = "Please login first with LOGIN username\n";
                            SocketWrapper::sendData(clientSocket, response.c_str(), response.length());
                        }
                    }
                    else if (upperMessage.find("JOIN") == 0)
                    {
                        // Format: JOIN gameId
                        size_t pos = message.find(" ");
                        if (pos != std::string::npos && clientId != "Unknown")
                        {
                            int sessionId = std::stoi(message.substr(pos + 1));
                            if (joinGameSession(sessionId, clientId))
                            {
                                gameSessionId = sessionId;

                                // Get the session and add this client's socket
                                GameSession *session = getGameSession(gameSessionId);
                                if (session)
                                {
                                    session->addClientSocket(clientSocket);
                                    session->broadcastGameState();
                                }

                                std::string response = "Joined game with ID: " + std::to_string(gameSessionId) + "\n";
                                SocketWrapper::sendData(clientSocket, response.c_str(), response.length());
                            }
                            else
                            {
                                std::string response = "Failed to join game\n";
                                SocketWrapper::sendData(clientSocket, response.c_str(), response.length());
                            }
                        }
                    }
                    else if (upperMessage.find("MOVE") == 0)
                    {
                        // Format: MOVE fromX fromY toX toY
                        if (gameSessionId != -1)
                        {
                            // Parse move coordinates
                            int fromX, fromY, toX, toY;
                            if (sscanf(message.c_str(), "%*[^0-9]%d %d %d %d", &fromX, &fromY, &toX, &toY) == 4)
                            {
                                GameSession *session = getGameSession(gameSessionId);
                                if (session)
                                {
                                    std::cout << "*** Before move call for " << clientId << " ***" << std::endl;

                                    bool moveResult = session->makeMove(clientId, fromX, fromY, toX, toY);

                                    std::cout << "*** After move call, result: " << (moveResult ? "success" : "failure") << " ***" << std::endl;

                                    if (!moveResult)
                                    {
                                        std::string response = "Invalid move\n";
                                        SocketWrapper::sendData(clientSocket, response.c_str(), response.length());
                                    }
                                }
                                else
                                {
                                    std::string response = "Game session not found\n";
                                    SocketWrapper::sendData(clientSocket, response.c_str(), response.length());
                                }
                            }
                            else
                            {
                                std::string response = "Invalid move format. Use: MOVE fromX fromY toX toY\n";
                                SocketWrapper::sendData(clientSocket, response.c_str(), response.length());
                            }
                        }
                        else
                        {
                            std::string response = "You are not in a game\n";
                            SocketWrapper::sendData(clientSocket, response.c_str(), response.length());
                        }
                    }
                    else if (upperMessage.find("STATE") == 0)
                    {
                        // Get current game state
                        if (gameSessionId != -1)
                        {
                            GameSession *session = getGameSession(gameSessionId);
                            if (session)
                            {
                                std::string state = session->getBoardState() + "\n";
                                SocketWrapper::sendData(clientSocket, state.c_str(), state.length());
                            }
                        }
                        else
                        {
                            std::string response = "You are not in a game\n";
                            SocketWrapper::sendData(clientSocket, response.c_str(), response.length());
                        }
                    }
                    else if (upperMessage.find("HELP") == 0)
                    {
                        // Send available commands
                        std::string response = "Available commands:\n";
                        response += "LOGIN username - Log in with a username\n";
                        response += "CREATE - Create a new game\n";
                        response += "JOIN gameId - Join an existing game\n";
                        response += "MOVE fromX fromY toX toY - Make a move\n";
                        response += "STATE - Get the current game state\n";
                        response += "HELP - Show this help message\n";
                        SocketWrapper::sendData(clientSocket, response.c_str(), response.length());
                    }
                    else
                    {
                        // Unknown command
                        std::string response = "Unknown command. Type HELP for available commands.\n";
                        SocketWrapper::sendData(clientSocket, response.c_str(), response.length());
                    }
                }
                catch (const std::exception &e)
                {
                    std::cerr << "Exception processing command: " << e.what() << std::endl;
                    std::string response = "Server error processing command\n";
                    SocketWrapper::sendData(clientSocket, response.c_str(), response.length());
                }
                catch (...)
                {
                    std::cerr << "Unknown exception processing command" << std::endl;
                    std::string response = "Server error processing command\n";
                    SocketWrapper::sendData(clientSocket, response.c_str(), response.length());
                }
            }
            catch (const std::exception &e)
            {
                std::cerr << "Exception in client communication loop: " << e.what() << std::endl;
            }
            catch (...)
            {
                std::cerr << "Unknown exception in client communication loop" << std::endl;
            }
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Exception in handleClientConnection: " << e.what() << std::endl;
    }
    catch (...)
    {
        std::cerr << "Unknown exception in handleClientConnection" << std::endl;
    }

    // Close the client socket
    SocketWrapper::closeSocket(clientSocket);
    std::cout << "Client disconnected: " << clientId << std::endl;
}

int Server::createGameSession(const std::string &player1Id)
{
    std::lock_guard<std::mutex> lock(sessionsMutex);

    // create invite code
    static const char alphanum[] =
    "0123456789"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz";
    std::string inviteCode;
    inviteCode.reserve(8);

    for (int i = 0; i < 8; ++i) {
        inviteCode += alphanum[rand() % (sizeof(alphanum) - 1)];
    }

    // Create a new game session
    int sessionId = nextSessionId++;
    GameSession *session = new GameSession(inviteCode, sessionId, player1Id);

    // Store it
    gameSessions[sessionId] = session;
    gameCodes[sessionId] = inviteCode;

    return sessionId;
}

bool Server::joinGameSession(int sessionId, const std::string &player2Id)
{
    std::lock_guard<std::mutex> lock(sessionsMutex);

    // Find the session
    auto it = gameSessions.find(sessionId);
    if (it == gameSessions.end())
    {
        return false; // Session not found
    }

    // Join the game
    return it->second->joinGame(player2Id);
}

GameSession *Server::getGameSession(int sessionId)
{
    std::lock_guard<std::mutex> lock(sessionsMutex);

    // Find the session
    auto it = gameSessions.find(sessionId);
    if (it == gameSessions.end())
    {
        return nullptr; // Session not found
    }

    return it->second;
}

void Server::closeSocket(socket_t socket)
{
    SocketWrapper::closeSocket(socket);
}