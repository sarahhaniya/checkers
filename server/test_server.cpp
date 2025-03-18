// server/test_server.cpp
#include "include/Server.h"
#include <iostream>
#include <thread>
#include <chrono>

#include "GameLogic/Board.h"
#include "GameLogic/Piece.h"
#include "GameLogic/Move.h"
#include "GameLogic/Typedefs.h"


// Add this at the beginning of your main() function
void testBlackPieceMoves()
{
    std::cout << "\n----- Testing Black Piece Moves -----\n"
              << std::endl;

    // Create a new board
    Board board;

    // Get a black piece
    Piece *blackPiece = board.getValueAt(1, 5);
    if (!blackPiece)
    {
        std::cout << "Error: No piece at (1,5)" << std::endl;
        return;
    }

    // Check if it's black
    if (blackPiece->isWhite)
    {
        std::cout << "Error: Piece at (1,5) is white" << std::endl;
        return;
    }

    std::cout << "Found black piece at (1,5)" << std::endl;

    // Try to generate moves
    try
    {
        moves_t moves = blackPiece->getAllPossibleMoves(board);
        std::cout << "Generated " << moves.size() << " possible moves" << std::endl;

        // Print the moves
        for (size_t i = 0; i < moves.size(); i++)
        {
            coords_t end = moves[i]->getEndingPosition();
            std::cout << "  Move " << i << ": To (" << end[0] << "," << end[1] << ")" << std::endl;
        }
    }
    catch (std::exception &e)
    {
        std::cout << "Exception: " << e.what() << std::endl;
    }
    catch (...)
    {
        std::cout << "Unknown exception occurred" << std::endl;
    }

    std::cout << "\n----- End Test -----\n"
              << std::endl;
}


void debugBlackPiece()
{
    std::cout << "\n--- BLACK PIECE DEBUG ---\n"
              << std::endl;

    // Create a new board
    Board board;

    // Get a black piece
    Piece *blackPiece = board.getValueAt(1, 5);
    if (!blackPiece)
    {
        std::cout << "Error: No piece at position (1,5)" << std::endl;
        return;
    }

    // Verify it's black
    if (blackPiece->isWhite)
    {
        std::cout << "Error: Piece at (1,5) is not black" << std::endl;
        return;
    }

    std::cout << "Analyzing black piece at (1,5)" << std::endl;

    // Try to generate moves
    try
    {
        std::cout << "Attempting to get possible moves..." << std::endl;
        moves_t moves = blackPiece->getAllPossibleMoves(board);

        std::cout << "Successfully generated " << moves.size() << " moves:" << std::endl;
        for (auto &move : moves)
        {
            coords_t endPos = move->getEndingPosition();
            std::cout << "  -> (" << endPos[0] << "," << endPos[1] << ")" << std::endl;
        }
    }
    catch (const std::exception &e)
    {
        std::cout << "Exception while generating moves: " << e.what() << std::endl;
    }
    catch (...)
    {
        std::cout << "Unknown exception while generating moves" << std::endl;
    }

    std::cout << "\n--- END BLACK PIECE DEBUG ---\n"
              << std::endl;
}
int main()
{
    testBlackPieceMoves();
    // Kill any previous instances of the server
    killPreviousInstances();

    // Create a server starting at port 8080 with 4 worker threads
    Server server(8080, 4);

    std::cout << "Starting server..." << std::endl;
    if (!server.start())
    {
        std::cerr << "Failed to start server" << std::endl;
        return 1;
    }

    // Create a test game session
    int sessionId = server.createGameSession("Player1");
    std::cout << "Created game session with ID: " << sessionId << std::endl;

    std::cout << "Server started. Use a tool like netcat to connect:" << std::endl;
    std::cout << "nc localhost " << server.getPort() << std::endl;
    std::cout << "\nTest commands:" << std::endl;
    std::cout << "LOGIN username - Log in with a username" << std::endl;
    std::cout << "CREATE - Create a new game" << std::endl;
    std::cout << "JOIN gameId - Join an existing game" << std::endl;
    std::cout << "MOVE fromX fromY toX toY - Make a move" << std::endl;
    std::cout << "STATE - Get the current game state" << std::endl;
    std::cout << "HELP - Show available commands" << std::endl;
    std::cout << "\nPress Ctrl+C to stop the server" << std::endl;

    // Keep the main thread alive
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}