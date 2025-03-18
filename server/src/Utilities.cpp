// server/src/Utilities.cpp
#include <iostream>

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
    // This function is called by HumanPlayer, but we'll implement it as a no-op
    // for the server version since we handle game end differently
}