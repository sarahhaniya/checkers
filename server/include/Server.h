// server/include/Server.h
#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <unordered_map>
#include "SocketWrapper.h"
#define _WEBSOCKETPP_CPP11_THREAD_

#define ASIO_STANDALONE
#include "../asio/asio/include/asio.hpp"
#include "../websocketpp/websocketpp/server.hpp"
#include "../websocketpp/websocketpp/config/asio_no_tls.hpp"
#include "../src/DatabaseManager.h"

// Declare the function before any class definitions
void killPreviousInstances();

class GameSession;
class ThreadPool;

class Server
{
private:
    int port;
    socket_t serverSocket; // Changed from int to socket_t
    std::atomic<bool> running;
    ThreadPool *threadPool;
    DatabaseManager dbManager;
    bool dbInitialized;
    int nextSessionId;

    std::mutex sessionsMutex;
    std::unordered_map<int, GameSession *> gameSessions;
    std::unordered_map<int, std::string > gameCodes;

    std::thread acceptThread;

    // Method for accept thread
    void acceptConnections();

    typedef websocketpp::server<websocketpp::config::asio> WebSocketServer;
    typedef WebSocketServer::message_ptr message_ptr;
    WebSocketServer wsServer;
    
    // WebSocket handlers
    void onWebSocketMessage(websocketpp::connection_hdl hdl, message_ptr msg);
    void onWebSocketOpen(websocketpp::connection_hdl hdl);
    void onWebSocketClose(websocketpp::connection_hdl hdl);
    
    // Map to track WebSocket connections to client IDs
    std::map<websocketpp::connection_hdl, std::string, std::owner_less<websocketpp::connection_hdl>> wsConnections;

    // User database: username -> (email, password)
std::unordered_map<std::string, std::pair<std::string, std::string>> registeredUsers;

public:
    Server(int port, int numThreads);
    ~Server();

    bool start();
    void stop();

    int createGameSession(const std::string &player1Id);
    bool joinGameSession(int sessionId, const std::string &player2Id);
    GameSession *getGameSession(int sessionId);

    void handleClientConnection(socket_t clientSocket); // Changed from int to socket_t

    // Utility method to safely close a socket
    static void closeSocket(socket_t socket); // Changed from int to socket_t

    int getPort() const { return port; }

    bool initializeDatabase();
    bool registerUser(const std::string& username, const std::string& email, const std::string& password);
    bool authenticateUser(const std::string& username, const std::string& password);
    bool saveGameState(int sessionId, const std::string& boardState);
    std::string loadGameState(int sessionId);
    bool recordMove(int sessionId, int fromX, int fromY, int toX, int toY, bool isWhite);

    std::string generateInviteCode();
    int findSessionByInviteCode(const std::string& gameCode);
    bool processGameMove(const std::string& username, int fromX, int fromY, int toX, int toY);
};

#endif // SERVER_H