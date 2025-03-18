// server/src/Session.cpp - update with board handling
#include "../include/Session.h"
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <sstream>

GameSession::GameSession(int id, const std::string &p1Id)
    : sessionId(id),
      player1Id(p1Id),
      gameStarted(false),
      gameBoard(), // Initialize a new board
      isPlayer1Turn(true)
{

    std::cout << "Game session " << id << " created with player: " << p1Id << std::endl;
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
    for (int socket : clientSockets)
    {
        close(socket);
    }

    std::cout << "Game session " << sessionId << " destroyed" << std::endl;
}

bool GameSession::joinGame(const std::string &p2Id)
{
    std::lock_guard<std::mutex> lock(gameMutex);

    // Check if game is already full
    if (!player2Id.empty())
    {
        return false;
    }

    // Set player 2
    player2Id = p2Id;
    gameStarted = true;

    std::cout << "Player " << p2Id << " joined game session " << sessionId << std::endl;

    return true;
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
            std::cout << "  -> (" << endPos[0] << "," << endPos[1] << ")" << std::endl;
        }

        // After calculating possibleMoves, add this code:

        // Check if any jumps are available
        bool jumpAvailable = false;
        for (auto &move : possibleMoves)
        {
            // We need to determine if the move is a jump
            std::vector<Piece *> jumpedPieces = move->getJumpedPieces(gameBoard);
            if (!jumpedPieces.empty())
            {
                jumpAvailable = true;
                std::cout << "Jump available from (" << fromX << "," << fromY
                          << ") to (" << move->getEndingPosition()[0] << ","
                          << move->getEndingPosition()[1] << ")" << std::endl;
            }
        }

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
                break;
            }
        }

        if (!moveFound)
        {
            std::cout << "Move to (" << toX << "," << toY << ") not found in possible moves" << std::endl;
            logMutexRelease("makeMove - invalid move");
            return false; // Move not found in possible moves
        }

        // If jumps are available but the selected move is not a jump, reject it
        if (jumpAvailable && !isJumpMove)
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
        for (int socket : clientSockets)
        {
            std::string message = gameState + "\n";
            send(socket, message.c_str(), message.length(), 0);
        }

        std::cout << "Broadcast complete" << std::endl;

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
    for (int socket : clientSockets)
    {
        std::string message = gameState + "\n";
        send(socket, message.c_str(), message.length(), 0);
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
            for (int socket : clientSockets)
            {
                std::string message = state + "\n";
                send(socket, message.c_str(), message.length(), 0);
            }
            std::cout << "Broadcast complete" << std::endl;
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

void GameSession::addClientSocket(int socket)
{
    std::lock_guard<std::mutex> lock(gameMutex);
    clientSockets.push_back(socket);
}
