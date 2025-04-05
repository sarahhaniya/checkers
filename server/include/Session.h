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
    std::string inviteCode;  // Move this before player1Id
    std::string player1Id;
    std::string player2Id;
    std::vector<socket_t> clientSockets;
    bool gameStarted;

    Board gameBoard; 
    std::atomic<bool> isPlayer1Turn;
    std::mutex gameMutex;

    static int mutexOperationId;
    void logMutexAcquire(const std::string &methodName);
    void logMutexRelease(const std::string &methodName);

    // Public WebSocketServer type
    typedef websocketpp::server<websocketpp::config::asio> WebSocketServer;
    
public:
    std::vector<std::pair<websocketpp::connection_hdl, WebSocketServer*>> wsConnections;

    GameSession(std::string inviteCode, int id, const std::string &p1Id);
    ~GameSession();

    bool joinGame(const std::string &p2Id);
    bool makeMove(const std::string &playerId, int fromX, int fromY, int toX, int toY);
    std::string getBoardState() const;
    std::string getBoardStateJson() const;

    void addClientSocket(socket_t socket);
    void broadcastGameState();

    int getSessionId() const { return sessionId; }
    bool isGameFull() const { return !player2Id.empty(); }
    bool hasStarted() const { return gameStarted; }

    bool forceBlackMove(int fromX, int fromY, int toX, int toY);
    const std::string &getPlayer2Id() const { return player2Id; }
    bool playerHasJumps(bool isWhiteTurn);

    const Board &getGameBoard() const { return gameBoard; }
    bool checkForWinner();

    // Declarations that match the implementation
    bool containsPlayer(const std::string& username) const;
    std::string getWhitePlayer() const;
    std::string getInviteCode() const;
    void addWebSocketHandle(websocketpp::connection_hdl hdl, WebSocketServer* server);
};

#endif // SESSION_H