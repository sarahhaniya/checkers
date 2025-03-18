#include "HumanPlayer.h"

#include "Board.h"
#include "Move.h"
#include "Piece.h"
#include "Typedefs.h"

#include <array>
#include <exception>
#include <iostream>
#include <cassert>

#include <algorithm> // for transform
#include <stdexcept> // for runtime_error

// forward declare utilities in main.cpp
void clearScreen();
void triggerEndGame();

/**
 * Gets a move, by asking the human player what move they want to do.
 * @param board The board to apply the move to (assumed to be oriented so that this player is on the top)
 * @return Returns the board, modified according to the player's move
 */
void HumanPlayer::getMove(Board& board)
{        
    // display board to help user (without possible moves)
    displayBoard(board);
    
    // keep asking until they select a piece with a valid move
    moves_t possibleMoves;
    while (true)
    {
        // ask user for a piece
        Piece* pieceMoving = getPieceFromUser(board);
                    
        // check for quit
        if (pieceMoving == nullptr)
            return;
        
        // find all possible moves the player could do
        possibleMoves = pieceMoving->getAllPossibleMoves(board);
        
        // check that there are some, and if so continue to ask for move
        if (possibleMoves.empty())
            std::cout << "That piece has no possible moves! Please choose another:" << '\n';
        else
        {
            // show the user possible moves and ask for one (user will enter a number)
            displayBoard(board, possibleMoves);
            move_ptr_t move = getMoveFromUser(possibleMoves);
            
            // apply move to board and return it if the user entered a valid one
            // OTHERWISE, the user requested a retry, so loop again
            if (move != nullptr)
            {
                board.applyMoveToBoard(move, pieceMoving);
                return;
            }
        }
    } 
}
    
/**
 * Responsible for displaying the game board to the user (optionally with possible moves)
 * @param board The board to be displayed
 * @param possibleMoves An optional std::vector of possible moves to display while printing the board.
 * The board will display as normal if this is null.
 */
void HumanPlayer::displayBoard(const Board& board, const moves_t possibleMoves)
{
    // clear the screen for board display
    clearScreen();
    
    // include a hidden top row for coordinates
    for (int y = -1; y < Board::SIZE; y++)
    {   
        // include a hidden left column for coordinates
        for (int x = -1; x < Board::SIZE; x++)
        {
            // add an exception for the top row (print letter coordinates)
            if (y == -1) 
            {
                if (x != -1) // skip hidden column
                    // print a letter, starting with capital a, for each x value
                    std::cout << "-" << (char)(x + 65) << "- ";
                else
                    std::cout << "     "; // still fill the place we skipped
            }
            // add an exception for the left column (print number coordinates)
            else if (x == -1)
            {
                if (y != -1) // skip hidden row
                    // print a number, starting with one, for each y value
                    std::cout << "-" << y + 1 << "- ";
            }
            else
            {
                // get piece here (possibly null)
                Piece* thisPiece = board.getValueAt(x, y);
                
                // if there are any, loop over the possible moves and see if any end at this space
                if (!possibleMoves.empty())
                {
                    // use to determine whether to continue and skip printing other things
                    bool moveFound = false;
                    
                    for (unsigned int i = 0; i < possibleMoves.size(); i++)
                    {
                        coords_t move = possibleMoves[i]->getEndingPosition();
                        if (move[0] == x && move[1] == y)
                        {
                            // if one here, put the list index (one-indexed) here as a char
                            std::cout << "| " << i+1 << " ";
                            moveFound = true;
                        }
                    }
                    
                    // if a move is found here, skip our other possible printings
                    if (moveFound)
                        continue;
                }
             
                // if the piece at this location exists, print it with a bar for cosmetics
                if (thisPiece != nullptr)
                    std::cout << "| " << thisPiece->getString();
                // print out dots (black places) at checkerboard spaces
                else if (board.isCheckerboardSpace(x, y))
                    std::cout << "| . ";
                else
                    std::cout << "|   ";
            }
        }
        std::cout << '\n';
    }
}

/**
 * Responsible for displaying the game board to the user (WITHOUT possible moves)
 * @param board The board to be displayed
 */
void HumanPlayer::displayBoard(const Board& board)
{
	// emptyPossibleMoves will never change now, but that's okay
	static moves_t emptyPossibleMoves(0);
	displayBoard(board, emptyPossibleMoves);
}

/**
 * Asks the user for a piece on the board (for them to move),
 * and ensures it is an actual piece of the correct color
 * @param board The board to check against
 * @return The Piece object to be returned (will be an actual piece)
 */
Piece *HumanPlayer::getPieceFromUser(const Board &board)
{
    while (true)
    {
        using namespace std;

        string raw;

        cout << getColor() << ", please select a piece by its coordinates (e.g., A3):" << '\n';
        try
        {
            getline(cin, raw);

            // Convert to lowercase to handle both upper and lower case
            transform(raw.begin(), raw.end(), raw.begin(), ::tolower);

            // Allow user to exit
            if (raw == "exit")
            {
                triggerEndGame();
                return nullptr;
            }

            // Ensure the input is exactly 2 characters long
            if (raw.length() != 2)
                throw runtime_error("Please enter a coordinate in the form '[letter][number]'.");

            // Get letter and number characters
            char letterChar = raw[0];
            char numberChar = raw[1];

            // Validate letter and number ranges
            if (letterChar < 'a' || letterChar > 'h')
                throw runtime_error("Please enter a letter between 'a' and 'h'.");
            if (numberChar < '1' || numberChar > '8')
                throw runtime_error("Please enter a number between '1' and '8'.");

            // Convert characters to 0-based coordinates
            int x = letterChar - 'a';
            int y = numberChar - '1';

            // Get the actual piece at the coordinates
            Piece *userPiece = board.getValueAt(x, y);

            // Validate the selected piece
            if (userPiece == nullptr)
                cout << "There is no piece at that position.\n"
                     << '\n';
            else if (userPiece->isWhite != this->isWhite)
                cout << "That's not your piece!\n"
                     << '\n';
            else
                return userPiece;
        }
        catch (const exception &e)
        {
            cout << e.what() << '\n';
        }
    }
}
/**
 * Asks the user for a number representing a move of a particular piece,
 * checking that it is an available move. (The user should be shown all moves beforehand)
 * @param possibleMoves The list of possible moves the user can request
 * @return The Move object representing the chosen move (may be null if the user chooses to get a new piece)
 */
move_ptr_t HumanPlayer::getMoveFromUser(const moves_t possibleMoves)
{
    int moveNum;

    while (true)
    {
        using namespace std;

        cout << getColor() << ", please select a move by its number (enter 0 to go back): ";
        try
        {
            cin >> moveNum;

            // Check for non-integer input
            if (cin.fail())
            {
                cin.clear();
                cin.ignore(32767, '\n');
                throw runtime_error("Please enter a number.");
            }

            // Ensure moveNum is non-negative and within valid range
            if (moveNum < 0 || moveNum > possibleMoves.size())
                throw runtime_error("Please enter a valid move number or 0 to go back.");

            // Allow user to go back by entering 0
            if (moveNum == 0)
                return nullptr;

            // Make sure cin is clean for the next input
            cin.ignore(32767, '\n');

            // Return the move the user entered (switch to 0-indexed)
            return possibleMoves[moveNum - 1];
        }
        catch (const exception &e)
        {
            cout << e.what() << '\n';
        }
    }
}
/**
 * @return Returns a titlecase string representing this player's color
 */
std::string HumanPlayer::getColor()
{
    return isWhite ? "White" : "Black";
}
