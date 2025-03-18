// server/include/Session.h
#ifndef SESSION_H
#define SESSION_H

#include <string>
#include <mutex>
#include <vector>
#include <atomic>
#include "../GameLogic/Board.h"
#include "../GameLogic/Move.h"
#include "../GameLogic/Piece.h"


class GameSession
{
private:
    int sessionId;
    std::string player1Id;
    std::string player2Id;
    std::vector<int> clientSockets;
    bool gameStarted;

    Board gameBoard; // The checkers board
    std::atomic<bool> isPlayer1Turn;
    std::mutex gameMutex;
    
    static int mutexOperationId;
    void logMutexAcquire(const std::string &methodName);
    void logMutexRelease(const std::string &methodName);

public:
    GameSession(int id, const std::string &p1Id);
    ~GameSession();

    bool joinGame(const std::string &p2Id);
    bool makeMove(const std::string &playerId, int fromX, int fromY, int toX, int toY);
    std::string getBoardState() const; // Return serialized board state

    void addClientSocket(int socket);
    void broadcastGameState();

    int getSessionId() const { return sessionId; }
    bool isGameFull() const { return !player2Id.empty(); }
    bool hasStarted() const { return gameStarted; }

    bool forceBlackMove(int fromX, int fromY, int toX, int toY);
    // Add this to the public section of GameSession class
    const std::string &getPlayer2Id() const { return player2Id; }
    bool playerHasJumps(bool isWhiteTurn);
};

#endif // SESSION_H