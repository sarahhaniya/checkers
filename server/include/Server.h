// server/include/Server.h
#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <unordered_map>
#include <netinet/in.h>

// Declare the function before any class definitions
void killPreviousInstances();

class GameSession;
class ThreadPool;



class Server
{
private:
    int port;
    int serverSocket;
    std::atomic<bool> running;
    ThreadPool *threadPool;

    std::mutex sessionsMutex;
    std::unordered_map<int, GameSession *> gameSessions;
    int nextSessionId;

    std::thread acceptThread;

    // Method for accept thread
    void acceptConnections();

public:
    Server(int port, int numThreads);
    ~Server();

    bool start();
    void stop();

    int createGameSession(const std::string &player1Id);
    bool joinGameSession(int sessionId, const std::string &player2Id);
    GameSession *getGameSession(int sessionId);

    void handleClientConnection(int clientSocket);

    // Utility method to safely close a socket
    static void closeSocket(int socket);

    int getPort() const { return port; }
};

#endif // SERVER_H