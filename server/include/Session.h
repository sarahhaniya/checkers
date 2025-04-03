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
#include "SocketWrapper.h"
#define _WEBSOCKETPP_CPP11_THREAD_

#define ASIO_STANDALONE

#include "../asio/asio/include/asio.hpp"
#include "../websocketpp/websocketpp/server.hpp"
#include "../websocketpp/websocketpp/config/asio_no_tls.hpp"

class GameSession
{
private:
    int sessionId;
    std::string player1Id;
    std::string player2Id;
    std::vector<socket_t> clientSockets; // Changed from int to socket_t
    bool gameStarted;

    Board gameBoard; // The checkers board
    std::atomic<bool> isPlayer1Turn;
    std::mutex gameMutex;

    static int mutexOperationId;
    void logMutexAcquire(const std::string &methodName);
    void logMutexRelease(const std::string &methodName);

    typedef websocketpp::server<websocketpp::config::asio> WebSocketServer;
    std::vector<std::pair<websocketpp::connection_hdl, WebSocketServer*>> wsConnections;

public:
    GameSession(std::string inviteCode, int id, const std::string &p1Id);
    ~GameSession();

    bool joinGame(const std::string &p2Id);
    bool makeMove(const std::string &playerId, int fromX, int fromY, int toX, int toY);
    std::string getBoardState() const; // Return serialized board state

    void addClientSocket(socket_t socket); // Changed from int to socket_t
    void broadcastGameState();

    int getSessionId() const { return sessionId; }
    bool isGameFull() const { return !player2Id.empty(); }
    bool hasStarted() const { return gameStarted; }

    bool forceBlackMove(int fromX, int fromY, int toX, int toY);
    const std::string &getPlayer2Id() const { return player2Id; }
    bool playerHasJumps(bool isWhiteTurn);

    const Board &getGameBoard() const { return gameBoard; }
    bool checkForWinner();

    // Add a method to add WebSocket handle
    void addWebSocketHandle(websocketpp::connection_hdl hdl, WebSocketServer* server) {
        wsConnections.push_back(std::make_pair(hdl, server));
    }

      // get JSON representation of board
      std::string getBoardStateJson() const;
};

#endif // SESSION_H