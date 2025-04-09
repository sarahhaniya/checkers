// server/src/Session.cpp - update with board handling
#include "../include/Session.h"
#include "../include/SocketWrapper.h"
#include <iostream>
#include <sstream>

#include "../include/nlohmann/json.hpp"
using nlohmann::json;

GameSession::GameSession(std::string inviteCode, int id, const std::string &p1Id, DatabaseManager* dbRef)
    : sessionId(id),
      player1Id(p1Id),
      gameStarted(false),
      gameBoard(), // Initialize a new board
      isPlayer1Turn(true),
      db(dbRef)
{
    std::cout << "Game session " << id << " created with player: " << p1Id << std::endl;
}

void GameSession::initializeBoard() {
    gameBoard.setupInitialBoard();  // new function you wrote in Board.cpp
}
void GameSession::resetBoard() {
    initializeBoard();        
    isPlayer1Turn = true;      // Player1 starts
    winner = -1;               // Clear winner
}

int GameSession::mutexOperationId = 0;

void GameSession::logMutexAcquire(const std::string &methodName)
{
    int opId = ++mutexOperationId;
    std::cout << "MUTEX [" << opId << "] ACQUIRING in " << methodName << std::endl;
}

void GameSession::logMutexRelease(const std::string &methodName)
{
    int opId = ++mutexOperationId;
    std::cout << "MUTEX [" << opId << "] RELEASED in " << methodName << std::endl;
}

GameSession::~GameSession()
{
    // Close all client sockets
    for (socket_t socket : clientSockets)
    {
        SocketWrapper::closeSocket(socket);
    }

    std::cout << "Game session " << sessionId << " destroyed" << std::endl;
}

bool GameSession::joinGame(const std::string &p2Id)
{
    std::lock_guard<std::mutex> lock(gameMutex);

    // Check if game is already full
    if (!player2Id.empty())
    {
        std::cout << "[JOIN FAILED] Game already has 2 players." << std::endl;
        return false;
    }

    // Prevent player1 from joining again
    if (p2Id == player1Id)
    {
        std::cout << "[JOIN FAILED] Player " << p2Id << " is already player1." << std::endl;
        return false;
    }

    // Set player 2
    player2Id = p2Id;
    gameStarted = true;

    std::cout << "Player " << p2Id << " joined game session " << sessionId << std::endl;

    return true;
}

bool GameSession::playerHasJumps(bool isWhiteTurn)
{
    // Loop through all board positions
    for (int y = 0; y < Board::SIZE; y++)
    {
        for (int x = 0; x < Board::SIZE; x++)
        {
            Piece *pieceAtPos = gameBoard.getValueAt(x, y);
            // If there's a piece and it belongs to the current player
            if (pieceAtPos && pieceAtPos->isWhite == isWhiteTurn)
            {
                // Get all possible moves
                moves_t pieceMoves = pieceAtPos->getAllPossibleMoves(gameBoard);
                for (auto &move : pieceMoves)
                {
                    // Check if any move is a jump
                    std::vector<Piece *> jumpedPieces = move->getJumpedPieces(gameBoard);
                    if (!jumpedPieces.empty())
                    {
                        std::cout << "Jump available for " << (isWhiteTurn ? "White" : "Black")
                                  << " piece at (" << x << "," << y << ")" << std::endl;
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

bool GameSession::makeMove(const std::string &playerId, int fromX, int fromY, int toX, int toY)
{
    logMutexAcquire("makeMove");
    std::lock_guard<std::mutex> lock(gameMutex);

    try
    {
        // Print debug info
        std::cout << "Attempting move by " << playerId << " from ("
                  << fromX << "," << fromY << ") to ("
                  << toX << "," << toY << ")" << std::endl;

        // Check if it's this player's turn
        bool isPlayer1 = (playerId == player1Id);
        if ((isPlayer1 && !isPlayer1Turn) || (!isPlayer1 && isPlayer1Turn))
        {
            std::cout << "Not " << playerId << "'s turn" << std::endl;
            logMutexRelease("makeMove - not player's turn");
            return false; // Not this player's turn
        }

        // Check bounds
        if (fromX < 0 || fromX >= Board::SIZE || fromY < 0 || fromY >= Board::SIZE ||
            toX < 0 || toX >= Board::SIZE || toY < 0 || toY >= Board::SIZE)
        {
            std::cout << "Move coordinates out of bounds" << std::endl;
            logMutexRelease("makeMove - out of bounds");
            return false;
        }

        // Get the piece at the starting position
        std::cout << "Getting piece at position (" << fromX << "," << fromY << ")" << std::endl;
        Piece *piece = gameBoard.getValueAt(fromX, fromY);

        // Check if piece exists
        if (piece == nullptr)
        {
            std::cout << "No piece at position (" << fromX << "," << fromY << ")" << std::endl;
            logMutexRelease("makeMove - no piece");
            return false;
        }

        // Check if piece belongs to the player
        if (piece->isWhite != isPlayer1)
        {
            std::cout << "Piece doesn't belong to " << playerId << std::endl;
            logMutexRelease("makeMove - wrong piece color");
            return false;
        }

        // Get all possible moves for this piece
        std::cout << "Generating moves for piece at (" << fromX << "," << fromY
                  << ") Color: " << (piece->isWhite ? "White" : "Black") << std::endl;

        std::cout << "About to call getAllPossibleMoves..." << std::endl;
        moves_t possibleMoves = piece->getAllPossibleMoves(gameBoard);
        std::cout << "getAllPossibleMoves returned successfully with "
                  << possibleMoves.size() << " moves" << std::endl;

        // Print possible moves for debugging
        std::cout << "Possible moves for piece at (" << fromX << "," << fromY << "): " << std::endl;
        for (auto &move : possibleMoves)
        {
            coords_t endPos = move->getEndingPosition();
            std::vector<Piece *> jumpedPieces = move->getJumpedPieces(gameBoard);
            bool isJumpMove = !jumpedPieces.empty();
            std::cout << "  -> (" << endPos[0] << "," << endPos[1] << ")"
                      << (isJumpMove ? " [JUMP]" : "") << std::endl;
        }

        // Add detailed debugging for ALL pieces with possible moves
        std::cout << "\n===== DEBUGGING ALL POSSIBLE MOVES =====\n";
        bool anyJumpAvailable = false;
        // Loop through all board positions
        for (int y = 0; y < Board::SIZE; y++)
        {
            for (int x = 0; x < Board::SIZE; x++)
            {
                Piece *pieceAtPos = gameBoard.getValueAt(x, y);
                // If there's a piece and it belongs to the current player
                if (pieceAtPos && pieceAtPos->isWhite == isPlayer1)
                {
                    // Get all possible moves
                    moves_t pieceMoves = pieceAtPos->getAllPossibleMoves(gameBoard);
                    if (!pieceMoves.empty())
                    {
                        std::cout << "Generating moves for piece at (" << x << "," << y
                                  << ") Color: " << (pieceAtPos->isWhite ? "White" : "Black")
                                  << std::endl;

                        bool hasJumps = false;
                        // Display each move
                        for (auto &m : pieceMoves)
                        {
                            coords_t endPos = m->getEndingPosition();
                            std::vector<Piece *> jumpedPieces = m->getJumpedPieces(gameBoard);
                            bool isJumpMove = !jumpedPieces.empty();
                            if (isJumpMove)
                            {
                                hasJumps = true;
                                anyJumpAvailable = true;
                            }

                            std::cout << "Found possible move to (" << endPos[0] << "," << endPos[1] << ")" << std::endl;
                        }

                        std::cout << "Total possible moves: " << pieceMoves.size() << std::endl;
                        std::cout << "Piece at (" << x << "," << y
                                  << ") Color: " << (pieceAtPos->isWhite ? "White" : "Black")
                                  << " has " << pieceMoves.size() << " moves:" << std::endl;

                        for (auto &m : pieceMoves)
                        {
                            coords_t endPos = m->getEndingPosition();
                            std::vector<Piece *> jumpedPieces = m->getJumpedPieces(gameBoard);
                            bool isJumpMove = !jumpedPieces.empty();

                            std::cout << "  -> To (" << endPos[0] << "," << endPos[1] << ")"
                                      << (isJumpMove ? " [JUMP]" : "") << std::endl;
                        }

                        if (hasJumps)
                        {
                            std::cout << "  ** This piece has jumps available! **" << std::endl;
                        }
                        std::cout << std::endl;
                    }
                }
            }
        }
        std::cout << "====================================\n\n";

        // Find if the requested move is valid
        std::cout << "Checking if requested move matches a possible move..." << std::endl;
        bool moveFound = false;
        move_ptr_t validMove;
        bool isJumpMove = false;

        for (auto &move : possibleMoves)
        {
            coords_t endPos = move->getEndingPosition();

            if (endPos[0] == toX && endPos[1] == toY)
            {
                validMove = move;
                moveFound = true;

                // Check if this move is a jump
                std::vector<Piece *> jumpedPieces = move->getJumpedPieces(gameBoard);
                isJumpMove = !jumpedPieces.empty();

                // If this is a jump, indicate it
                if (isJumpMove)
                {
                    std::cout << "Jump available from (" << fromX << "," << fromY
                              << ") to (" << toX << "," << toY << ")" << std::endl;
                }
                break;
            }
        }

        if (!moveFound)
        {
            std::cout << "Move to (" << toX << "," << toY << ") not found in possible moves" << std::endl;
            logMutexRelease("makeMove - invalid move");
            return false; // Move not found in possible moves
        }

        // If ANY jump is available but the selected move is not a jump, reject it
        if (anyJumpAvailable && !isJumpMove)
        {
            std::cout << "Jump is available, must take jump move" << std::endl;
            logMutexRelease("makeMove - jump required");
            return false;
        }

        // Apply the move
        std::cout << "Applying move to board..." << std::endl;
        gameBoard.applyMoveToBoard(validMove, piece);
        std::cout << "Move applied successfully" << std::endl;

        // Toggle turn
        isPlayer1Turn = !isPlayer1Turn;

        std::cout << "Move successful, turn is now "
                  << (isPlayer1Turn ? "Player1" : "Player2") << std::endl;

        // Capture the game state while the mutex is still held
        std::string gameState = getBoardState();

        // Log release of mutex before network operations
        logMutexRelease("makeMove - before broadcast");

        // The lock_guard will be automatically released when we exit this scope
        // End of mutex-protected section

        // Broadcast the game state - OUTSIDE the mutex lock
        for (socket_t socket : clientSockets)
        {
            std::string message = gameState + "\n";
            SocketWrapper::sendData(socket, message.c_str(), message.length());
        }

        std::cout << "Broadcast complete" << std::endl;

        // Check if there's a winner
        checkForWinner();

        return true;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Exception in makeMove: " << e.what() << std::endl;
        logMutexRelease("makeMove - exception");
        return false;
    }
    catch (...)
    {
        std::cerr << "Unknown exception in makeMove" << std::endl;
        logMutexRelease("makeMove - unknown exception");
        return false;
    }
}
void GameSession::markGameAsAbandonedBy(const std::string& leaverId) {
    std::lock_guard<std::mutex> lock(gameMutex);

    std::string message = "Player " + leaverId + " has left the game. Game is now over.";
    std::cout << "[MARK ABANDONED] Sending message: " << message << std::endl;

    gameStarted = false;  // mark session inactive
    winner = -1;

    // Notify all sockets (TCP + WebSocket)
    for (socket_t socket : clientSockets) {
        SocketWrapper::sendData(socket, message.c_str(), message.length());
    }

    for (auto& conn : GameSession::wsConnections) {
        try {
            conn.second->send(conn.first, message, websocketpp::frame::opcode::text);
        } catch (...) {
            // ignore
        }
    }

    std::cout << "[GameSession] Game ended because player " << leaverId << " left.\n";
}


void GameSession::broadcastGameState()
{
    logMutexAcquire("broadcastGameState");
    std::lock_guard<std::mutex> lock(gameMutex);

    // Get the game state while mutex is held
    std::string gameState = getBoardState();

    // Log release of mutex before network operations
    logMutexRelease("broadcastGameState - before send");

    // The lock_guard will be automatically released when we exit this scope
    // End of mutex-protected section

    // Send to all connected clients - OUTSIDE the mutex lock
    for (socket_t socket : clientSockets)
    {
        std::string message = gameState + "\n";
        SocketWrapper::sendData(socket, message.c_str(), message.length());
    }

              // Create a JSON representation of the game state
              std::string boardJson = getBoardStateJson();
           
              
              // Send to all WebSocket connections
              for (auto& conn : GameSession::wsConnections) {
                  try {
                      conn.second->send(conn.first, boardJson, websocketpp::frame::opcode::text);
                  } catch (const websocketpp::exception& e) {
                      // Handle errors
                  }
              }

    std::cout << "Broadcast completed" << std::endl;
}

// Implement in Session.cpp
bool GameSession::forceBlackMove(int fromX, int fromY, int toX, int toY)
{
    std::lock_guard<std::mutex> lock(gameMutex);

    try
    {
        std::cout << "Force-moving black piece from (" << fromX << "," << fromY
                  << ") to (" << toX << "," << toY << ")" << std::endl;

        // Get the piece
        Piece *piece = gameBoard.getValueAt(fromX, fromY);
        if (!piece || piece->isWhite)
        {
            std::cout << "No black piece at the specified position" << std::endl;
            return false;
        }

        // Create a move directly
        move_ptr_t move(new Move(fromX, fromY, toX, toY, nullptr, false));

        // Apply it to the board
        gameBoard.applyMoveToBoard(move, piece);

        // Toggle turn
        isPlayer1Turn = !isPlayer1Turn;

        std::cout << "Force-move successful" << std::endl;

        // Broadcast the state update
        try
        {
            std::string state = getBoardState();
            for (socket_t socket : clientSockets)
            {
                std::string message = state + "\n";
                SocketWrapper::sendData(socket, message.c_str(), message.length());
            }
            std::cout << "Broadcast complete" << std::endl;

            // Check if there's a winner
            checkForWinner();
        }
        catch (const std::exception &e)
        {
            std::cout << "Error broadcasting state: " << e.what() << std::endl;
        }

        return true;
    }
    catch (const std::exception &e)
    {
        std::cout << "Exception in forceBlackMove: " << e.what() << std::endl;
        return false;
    }
    catch (...)
    {
        std::cout << "Unknown exception in forceBlackMove" << std::endl;
        return false;
    }
}

std::string GameSession::getBoardState() const
{
    std::stringstream ss;

    ss << "Game " << sessionId << " - ";
    ss << "Player1: " << player1Id << ", ";
    ss << "Player2: " << player2Id << ", ";
    ss << "Turn: " << (isPlayer1Turn ? "Player1" : "Player2") << "\n\n";

    // Add ASCII board representation
    ss << "  0 1 2 3 4 5 6 7\n";
    for (int y = 0; y < Board::SIZE; y++)
    {
        ss << y << " ";
        for (int x = 0; x < Board::SIZE; x++)
        {
            Piece *piece = gameBoard.getValueAt(x, y);
            if (piece != nullptr)
            {
                // Show piece type - white (W) or black (B)
                ss << (piece->isWhite ? "W" : "B");

                // Check if it's a king using the piece's getString() method
                // which should include 'K' for kings
                if (piece->getString().find("K") != std::string::npos)
                {
                    ss << "K";
                }
                else
                {
                    ss << " ";
                }
            }
            else if (gameBoard.isCheckerboardSpace(x, y))
            {
                ss << ". ";
            }
            else
            {
                ss << "  ";
            }
        }
        ss << "\n";
    }

    return ss.str();
}

std::string GameSession::getBoardStateJson() const {
    std::stringstream ss;
    ss << "{";
    ss << "\"type\":\"game_joined\",";
    ss << "\"gameId\":\"" << sessionId << "\",";
    
    ss << "\"gameInfo\":{";
    ss << "\"player1Id\":\"" << player1Id << "\",";
    ss << "\"player2Id\":\"" << player2Id << "\",";
    ss << "\"currentTurn\":\"" << (isPlayer1Turn ? "Player1" : "Player2") << "\"";
    ss << "},";

    ss << "\"board\":[";
    if (gameStarted) {
        for (int y = 0; y < Board::SIZE; ++y) {
            ss << "[";
            for (int x = 0; x < Board::SIZE; ++x) {
                Piece* piece = gameBoard.getValueAt(x, y);
                if (piece) {
                    ss << "{";
                    ss << "\"isWhite\":" << (piece->isWhite ? "true" : "false") << ",";
                    ss << "\"isKing\":" << (piece->getString().find("K") != std::string::npos ? "true" : "false");
                    ss << "}";
                } else {
                    ss << "null";
                }
                if (x < Board::SIZE - 1) ss << ",";
            }
            ss << "]";
            if (y < Board::SIZE - 1) ss << ",";
        }
    }
    ss << "]";

    ss << "}";

    return ss.str();
}



void GameSession::addClientSocket(socket_t socket)
{
    std::lock_guard<std::mutex> lock(gameMutex);
    clientSockets.push_back(socket);
}

bool GameSession::checkForWinner()
{
    int whiteCount = 0;
    int blackCount = 0;

    // Count all pieces on the board
    for (int y = 0; y < Board::SIZE; y++)
    {
        for (int x = 0; x < Board::SIZE; x++)
        {
            Piece *pieceAtPos = gameBoard.getValueAt(x, y);
            if (pieceAtPos != nullptr)
            {
                if (pieceAtPos->isWhite)
                    whiteCount++;
                else
                    blackCount++;
            }
        }
    }

    std::cout << "Piece count - White: " << whiteCount << ", Black: " << blackCount << std::endl;

    // If either player has no pieces left, the other player wins
    if (whiteCount == 0 || blackCount == 0)
    {
        std::string message;
        if (whiteCount == 0)
            message = "BLACK WINS! Player " + player2Id + " is victorious!\n";
            if (db) {
                db->incrementWins(player2Id);
                db->incrementLosses(player1Id);
            }        
            else {
            message = "WHITE WINS! Player " + player1Id + " is victorious!\n";
            if (db) {
                db->incrementWins(player1Id);
                db->incrementLosses(player2Id);
            }        
            }
        std::cout << message;

        // Broadcast the win message to all clients
            for (socket_t socket : clientSockets)
            {
                SocketWrapper::sendData(socket, message.c_str(), message.length());
            }

            for (auto& conn : GameSession::wsConnections)
            {
                try {
                    conn.second->send(conn.first, message, websocketpp::frame::opcode::text);
                } catch (const websocketpp::exception& e) {
                    std::cerr << "WebSocket send error: " << e.what() << std::endl;
                }
            }


        return true;
    }

    return false;
}



int GameSession::getCurrentTurn() {
    return isPlayer1Turn ? 0 : 1;
}


