// server/src/Server.cpp
#include "../include/Server.h"
#include "../include/ThreadPool.h"
#include "../include/Session.h"
#include "../include/SocketWrapper.h"
#include "../websocketpp/websocketpp/config/asio_no_tls.hpp"
#include "../websocketpp/websocketpp/server.hpp"
#include "../websocketpp/websocketpp/common/asio.hpp"

#include <iostream>
#include <cstring>
#include <thread>
#include <chrono>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>

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
      nextSessionId(1),
      dbInitialized(false)
{
    // Initialize socket library (Windows needs this)
    SocketWrapper::initialize();

    // Create the thread pool
    threadPool = new ThreadPool(numThreads);
    dbInitialized = dbManager.initialize();
    if (!dbInitialized) {
        std::cerr << "Warning: Database initialization failed" << std::endl;
    } else {
        std::cout << "Database initialized successfully" << std::endl;
    }
}

class SHA256 {
    public:
    static inline unsigned int rotr(unsigned int x, unsigned int n) {
        return (x >> n) | (x << (32 - n));
    }
        static std::string hash(const std::string &input);
    private:
        static constexpr unsigned int k[64] = {
            0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,
            0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
            0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,
            0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
            0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,
            0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
            0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,
            0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
            0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,
            0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
            0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,
            0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
            0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,
            0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
            0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,
            0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
        };
    
        static void transform(const unsigned char *message, unsigned int block_nb, unsigned int *digest);
    };

    std::string SHA256::hash(const std::string &input) {
        unsigned int h[8] = {
            0x6a09e667,
            0xbb67ae85,
            0x3c6ef372,
            0xa54ff53a,
            0x510e527f,
            0x9b05688c,
            0x1f83d9ab,
            0x5be0cd19
        };
    
        std::vector<unsigned char> msg(input.begin(), input.end());
        size_t original_len = msg.size();
    
        msg.push_back(0x80);
        while ((msg.size() + 8) % 64 != 0) {
            msg.push_back(0);
        }
    
        uint64_t bit_len = original_len * 8;
        for (int i = 7; i >= 0; --i)
            msg.push_back((bit_len >> (i * 8)) & 0xFF);
    
        unsigned int w[64];
        for (size_t i = 0; i < msg.size(); i += 64) {
            for (int j = 0; j < 16; ++j)
                w[j] = (msg[i + j * 4] << 24) | (msg[i + j * 4 + 1] << 16) |
                       (msg[i + j * 4 + 2] << 8) | (msg[i + j * 4 + 3]);
    
            for (int j = 16; j < 64; ++j) {
                unsigned int s0 = rotr(w[j - 15], 7) ^ rotr(w[j - 15], 18) ^ (w[j - 15] >> 3);
                unsigned int s1 = rotr(w[j - 2], 17) ^ rotr(w[j - 2], 19) ^ (w[j - 2] >> 10);
                w[j] = w[j - 16] + s0 + w[j - 7] + s1;
            }
    
            unsigned int a = h[0], b = h[1], c = h[2], d = h[3];
            unsigned int e = h[4], f = h[5], g = h[6], h_ = h[7];
    
            for (int j = 0; j < 64; ++j) {
                unsigned int S1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
                unsigned int ch = (e & f) ^ ((~e) & g);
                unsigned int temp1 = h_ + S1 + ch + k[j] + w[j];
                unsigned int S0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
                unsigned int maj = (a & b) ^ (a & c) ^ (b & c);
                unsigned int temp2 = S0 + maj;
    
                h_ = g;
                g = f;
                f = e;
                e = d + temp1;
                d = c;
                c = b;
                b = a;
                a = temp1 + temp2;
            }
    
            h[0] += a; h[1] += b; h[2] += c; h[3] += d;
            h[4] += e; h[5] += f; h[6] += g; h[7] += h_;
        }
    
        std::stringstream ss;
        for (int i = 0; i < 8; ++i)
            ss << std::hex << std::setw(8) << std::setfill('0') << h[i];
    
        return ss.str();
    }

    std::string hashPassword(const std::string& password) {
        return SHA256::hash(password);
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
         if (SocketWrapper::bindSocket(serverSocket, 12345))
         {
             // Successfully bound to a port
             port = 12345; // Update the port to the one we actually bound to
             bound = true;
             break;
         }
 
         std::cerr << "Failed to bind to port " << (12345)
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
        asio::ip::tcp::endpoint endpoint(asio::ip::tcp::v4(), 8080);
        wsServer.listen(endpoint);
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

void Server::recordWin(const std::string& username) {
    if (dbInitialized) dbManager.incrementWins(username);
}

void Server::recordLoss(const std::string& username) {
    if (dbInitialized) dbManager.incrementLosses(username);
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
    std::istringstream iss(message); 

    try {
        if (upperMessage.find("LOGIN") == 0) {
            std::istringstream iss(message);
            std::string command, username, password;
            iss >> command >> username >> password;  

            std::cout << "[LOGIN] Trying username: " << username << std::endl;

    if (!username.empty() && !password.empty()) {
        std::string hashed = SHA256::hash(password);
        std::cout << "[LOGIN] Password (hashed): " << hashed << std::endl;

        if (dbManager.validateUser(username, hashed)) {
            wsConnections[hdl] = username;

            
            int wins = dbManager.getWins(username);
            int losses = dbManager.getLosses(username);


            std::ostringstream oss;
            oss << "{ \"type\": \"login_success\", \"username\": \"" << username
                << "\", \"wins\": " << wins
                << ", \"losses\": " << losses << " }";
        
            response = oss.str();
                } else {
            response = "{ \"type\": \"error\", \"message\": \"Invalid username or password\" }";
        }
        } else {
            response = "{ \"type\": \"error\", \"message\": \"Invalid login format\" }";
        }
    
        } else if (upperMessage.find("REGISTER") == 0) {
            // Format: REGISTER email username password
            std::string command, email, username, password;
            iss >> command >> email >> username >> password;
                
            if (registerUser(username, email, password)) {
                response = "{ \"type\": \"register_success\", \"username\": \"" + username + "\" }";
            } else {
                response = "{ \"type\": \"error\", \"message\": \"Registration failed. Username may already exist.\" }";
            }  
              
            } else if (upperMessage.find("CREATE") == 0) {
            std::string clientId = wsConnections[hdl];
            if (clientId != "Unknown") {
                if (sessionByPlayerId.count(clientId)) {
                    auto oldSession = sessionByPlayerId[clientId];
                    oldSession->markGameAsAbandonedBy(clientId);
                    sessionByPlayerId.erase(clientId);
                }
                int gameSessionId = createGameSession(clientId);
                sessionByPlayerId[clientId] = gameSessions[gameSessionId];

                GameSession* session = getGameSession(gameSessionId);
                if (session) {
                    session->addWebSocketHandle(hdl, &wsServer);
                     session->resetBoard();                
                     session->broadcastGameState();        
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
                    int sessionId = -1;
                    std::cout << "Looking up session code: " << code << std::endl;
std::cout << "Current gameCodes map:\n";
for (const auto& [id, gcode] : gameCodes) {
    std::cout << "  Session ID: " << id << " => Code: " << gcode << std::endl;
}
for (const auto& [key, value] : gameCodes){
    if (value == code){
        sessionId = key;
        break;
    }
}

if (sessionId == -1 || !gameSessions.count(sessionId)) {
    response = "{ \"type\": \"error\", \"message\": \"Invalid or expired game code\" }";
    wsServer.send(hdl, response, websocketpp::frame::opcode::text);
    return;
}
                    // int sessionId = 0;
                    // std::cout << "Client " << clientId << " attempting to join with code " << code << std::endl;

                    // for (const auto& [key, value] : gameCodes){
                    //  if (value == code){
                    //   sessionId = key;
                    //  }
                    // }
                    std::cout << "Trying to JOIN session with code: " << code << std::endl;
                    std::cout << "Resolved session ID: " << sessionId << std::endl;
                    std::cout << "Client ID: " << clientId << std::endl;
                    // if (!gameSessions.count(sessionId)) {
                    //     std::cout << " Session ID " << sessionId << " not found in gameSessions!" << std::endl;
                    // }
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
                std::string command;
                std::string username;
                std::string password;
                std::istringstream iss(message);
                iss >> command >> username >> password;

                if (authenticateUser(username, password)) {
                    int wins = dbManager.getWins(username);
                    int losses = dbManager.getLosses(username);
                    std::cout << "[LOGIN] Wins: " << wins << ", Losses: " << losses << std::endl;
                    
                    response = "{ \"type\": \"login_success\", \"username\": \"" + username +
                               "\", \"wins\": " + std::to_string(wins) +
                               ", \"losses\": " + std::to_string(losses) + " }";
                }
                
            }
        }
        
        else if (upperMessage.find("MOVE") == 0) {
    // Format: MOVE fromX fromY toX toY
    std::string clientId = wsConnections[hdl];
    int gameSessionId = -1;

  for (const auto& [id, session] : gameSessions) {
    if (session->getPlayer2Id() == clientId || session->getPlayer1Id() == clientId) {
        gameSessionId = id;
        break;
    }
}

    if (clientId != "Unknown" && gameSessionId != -1) {
        int fromX, fromY, toX, toY;
        if (sscanf(message.c_str(), "%*[^0-9]%d %d %d %d", &fromX, &fromY, &toX, &toY) == 4) {
            GameSession* session = getGameSession(gameSessionId);
            if (session) {
                bool moveResult = session->makeMove(clientId, fromX, fromY, toX, toY);

             std::ostringstream moveJson;
            moveJson << "{";
            moveJson << "\"type\":\"MoveResult\",";
            moveJson << "\"success\":" << (moveResult ? "true" : "false") << ",";
            moveJson << "\"from\":[" << fromX << "," << fromY << "],";
            moveJson << "\"to\":[" << toX << "," << toY << "],";
            moveJson << "\"board\":" << session->getBoardStateJson() << ",";  // Keep this if getBoardState() still returns json
            moveJson << "\"nextTurn\":" << session->getCurrentTurn();
            moveJson << "}";

            std::string jsonStr = moveJson.str();

                // Send to all players in the session
                for (auto& conn : session->getWsConnections()) {
                    try {
                        conn.second->send(conn.first, jsonStr, websocketpp::frame::opcode::text);
                    } catch (const websocketpp::exception& e) {
                        std::cerr << "WebSocket send failed: " << e.what() << std::endl;
                    }
                }
            }
        } else {
            response = "{ \"type\": \"error\", \"message\": \"Invalid move format. Use: MOVE fromX fromY toX toY\" }";
            wsServer.send(hdl, response, websocketpp::frame::opcode::text);
        }
    } else {
        response = "{ \"type\": \"error\", \"message\": \"You are not in a game\" }";
        wsServer.send(hdl, response, websocketpp::frame::opcode::text);
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
                                std::string state = session->getBoardStateJson() + "\n";
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
    GameSession* session = new GameSession(inviteCode, sessionId, player1Id, &dbManager);  // pass dbManager

    // Store it
    gameSessions[sessionId] = session;
    gameCodes[sessionId] = inviteCode;

    return sessionId;
}

bool Server::registerUser(const std::string& username, const std::string& email, const std::string& password) {
    std::string hashed = SHA256::hash(password);
    return dbManager.createUser(username, email, hashed);
}

bool Server::authenticateUser(const std::string& username, const std::string& password) {
    std::string hashed = SHA256::hash(password);
    return dbManager.validateUser(username, hashed);
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