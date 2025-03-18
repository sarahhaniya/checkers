// server/src/Server.cpp
#include "../include/Server.h"
#include "../include/ThreadPool.h"
#include "../include/Session.h"

#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

void killPreviousInstances()
{
    // Get current process ID
    pid_t currentPid = getpid();

    // Command to kill previous instances but exclude the current process
    std::string cmd = "ps -ef | grep test_server | grep -v grep | grep -v " +
                      std::to_string(currentPid) +
                      " | awk '{print $2}' | xargs -I {} kill -9 {} 2>/dev/null";

    system(cmd.c_str());

    // Give a moment for processes to terminate
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

Server::Server(int port, int numThreads)
    : port(port),
      serverSocket(-1),
      running(false),
      nextSessionId(1)
{

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
}

bool Server::start()
{
    // Create socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0)
    {
        std::cerr << "Failed to create socket: " << strerror(errno) << std::endl;
        return false;
    }

    // Set socket options
    int opt = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        std::cerr << "Failed to set socket options: " << strerror(errno) << std::endl;
        closeSocket(serverSocket);
        return false;
    }

    // Try to bind to the initial port, and if that fails, try subsequent ports
    const int MAX_PORT_ATTEMPTS = 10;
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;

    bool bound = false;
    for (int attempt = 0; attempt < MAX_PORT_ATTEMPTS; attempt++)
    {
        address.sin_port = htons(port + attempt);

        if (bind(serverSocket, (struct sockaddr *)&address, sizeof(address)) == 0)
        {
            // Successfully bound to a port
            port = port + attempt; // Update the port to the one we actually bound to
            bound = true;
            break;
        }

        std::cerr << "Failed to bind to port " << (port + attempt)
                  << ": " << strerror(errno) << std::endl;
    }

    if (!bound)
    {
        std::cerr << "Failed to bind to any port after " << MAX_PORT_ATTEMPTS << " attempts" << std::endl;
        closeSocket(serverSocket);
        return false;
    }

    // Listen for connections
    if (listen(serverSocket, 10) < 0)
    {
        std::cerr << "Failed to listen: " << strerror(errno) << std::endl;
        closeSocket(serverSocket);
        return false;
    }

    // Mark as running
    running = true;

    // Start accept thread
    acceptThread = std::thread(&Server::acceptConnections, this);

    std::cout << "Server started on port " << port << std::endl;
    return true;
}
void Server::stop()
{
    // Stop accepting new connections
    running = false;

    // Close server socket
    if (serverSocket >= 0)
    {
        closeSocket(serverSocket);
        serverSocket = -1;
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

    std::cout << "Server stopped" << std::endl;
}

void Server::acceptConnections()
{
    struct sockaddr_in clientAddress;
    socklen_t clientAddressLength = sizeof(clientAddress);

    while (running)
    {
        // Accept a connection
        int clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddress, &clientAddressLength);

        if (clientSocket < 0)
        {
            // Check if the error is because we're shutting down
            if (!running)
            {
                break;
            }

            std::cerr << "Failed to accept connection: " << strerror(errno) << std::endl;
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

void Server::handleClientConnection(int clientSocket)
{
    const int bufferSize = 1024;
    char buffer[bufferSize];
    std::string clientId = "Unknown"; // Will be set during authentication
    int gameSessionId = -1;           // -1 means not in a game

    try
    {
        // Send welcome message
        std::string welcome = "Welcome to Checkers Server\n";
        send(clientSocket, welcome.c_str(), welcome.length(), 0);

        while (running)
        {
            try
            {
                // Read data from client
                ssize_t bytesRead = recv(clientSocket, buffer, bufferSize - 1, 0);

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
                            send(clientSocket, response.c_str(), response.length(), 0);
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
                            send(clientSocket, response.c_str(), response.length(), 0);
                        }
                        else
                        {
                            std::string response = "Please login first with LOGIN username\n";
                            send(clientSocket, response.c_str(), response.length(), 0);
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
                                send(clientSocket, response.c_str(), response.length(), 0);
                            }
                            else
                            {
                                std::string response = "Failed to join game\n";
                                send(clientSocket, response.c_str(), response.length(), 0);
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
                                        send(clientSocket, response.c_str(), response.length(), 0);
                                    }
                                }
                                else
                                {
                                    std::string response = "Game session not found\n";
                                    send(clientSocket, response.c_str(), response.length(), 0);
                                }
                            }
                            else
                            {
                                std::string response = "Invalid move format. Use: MOVE fromX fromY toX toY\n";
                                send(clientSocket, response.c_str(), response.length(), 0);
                            }
                        }
                        else
                        {
                            std::string response = "You are not in a game\n";
                            send(clientSocket, response.c_str(), response.length(), 0);
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
                                send(clientSocket, state.c_str(), state.length(), 0);
                            }
                        }
                        else
                        {
                            std::string response = "You are not in a game\n";
                            send(clientSocket, response.c_str(), response.length(), 0);
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
                        send(clientSocket, response.c_str(), response.length(), 0);
                    }
                    else
                    {
                        // Unknown command
                        std::string response = "Unknown command. Type HELP for available commands.\n";
                        send(clientSocket, response.c_str(), response.length(), 0);
                    }
                }
                catch (const std::exception &e)
                {
                    std::cerr << "Exception processing command: " << e.what() << std::endl;
                    std::string response = "Server error processing command\n";
                    send(clientSocket, response.c_str(), response.length(), 0);
                }
                catch (...)
                {
                    std::cerr << "Unknown exception processing command" << std::endl;
                    std::string response = "Server error processing command\n";
                    send(clientSocket, response.c_str(), response.length(), 0);
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
    closeSocket(clientSocket);
    std::cout << "Client disconnected: " << clientId << std::endl;
}


int Server::createGameSession(const std::string &player1Id)
{
    std::lock_guard<std::mutex> lock(sessionsMutex);

    // Create a new game session
    int sessionId = nextSessionId++;
    GameSession *session = new GameSession(sessionId, player1Id);

    // Store it
    gameSessions[sessionId] = session;

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


void Server::closeSocket(int socket)
{
    close(socket);
}