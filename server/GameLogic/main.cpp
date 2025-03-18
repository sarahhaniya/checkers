#include "Player.h"
#include "HumanPlayer.h"
#include "Board.h"
#include "Piece.h"
#include "Move.h"

#include <vector>
#include <iostream>
#include <memory> // Add this include for unique_ptr

/**
 * File responsible for running the 2-player checkers game.
 *
 * @author Mckenna Cisler
 * @version 5.18.2016
 */

bool isPlayer1 = true;
bool endGameNow = false; // an easily accessible "end" variable

/**
 * Clears the terminal screen
 */
void clearScreen()
{
	// see http://stackoverflow.com/a/32008479/3155372
	std::cout << "\033[2J\033[1;1H";
}

/**
 * Responsible for quickly ending the game
 */
void triggerEndGame()
{
	endGameNow = true;
}

/**
 * Determines whether the game has been completed, or is in a stalemate
 * @param board The board to check to determine if we're at an endgame point.
 */
bool endGame(const Board &board)
{
	// have an emergency trigger for endgame
	if (endGameNow)
		return true;
	else
	{
		// otherwise search the board for pieces of both colors, and if none of one color are present,
		// the other player has won.
		int movableWhiteNum = 0;
		int movableBlackNum = 0;
		for (int pos = 0; pos < Board::SIZE * Board::SIZE; pos++)
		{
			// make sure the piece exists, and if so sum movable pieces for each color)
			Piece *pieceHere = board.getValueAt(pos);
			if (pieceHere != nullptr)
			{
				// only consider piece if it has possible moves
				if (!pieceHere->getAllPossibleMoves(board).empty())
				{
					if (pieceHere->isWhite)
						movableWhiteNum++;
					else
						movableBlackNum++;
				}
			}
		}

		using namespace std;

		// determine if anyone won (or if no one had any moves left)
		if (movableWhiteNum + movableBlackNum == 0)
			cout << "The game was a stalemate..." << endl;
		else if (movableWhiteNum == 0)
			cout << "Congratulations, Black, you have won the game gloriously!" << endl;
		else if (movableBlackNum == 0)
			cout << "Congratulations, White, you have won the game gloriously!" << endl;
		else
			return false;

		// we can only make it here if any of the above conditions are hit
		return true;
	}
}
int main()
{
	try
	{
		// Generate basic board and setup
		Board board;

		// Define players using unique_ptr for automatic memory management
		std::unique_ptr<Player> player1(new HumanPlayer(true));	 // White player
		std::unique_ptr<Player> player2(new HumanPlayer(false)); // Black player

		while (!endGame(board))
		{
			if (isPlayer1)
				player1->getMove(board);
			else
				player2->getMove(board);

			// Switch players
			isPlayer1 = !isPlayer1;
		}
	}
	catch (const std::exception &e)
	{
		std::cout << "An error occurred: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}