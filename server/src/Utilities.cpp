// server/src/Utilities.cpp
#include <iostream>

/**
 * Clears the terminal screen in a cross-platform way
 */
void clearScreen()
{
#ifdef _WIN32
    // Windows-specific clear screen
    system("cls");
#else
    // UNIX/Linux/MacOS terminal clear
    std::cout << "\033[2J\033[1;1H";
#endif
}

/**
 * Responsible for quickly ending the game
 */
void triggerEndGame()
{
    // This function is called by HumanPlayer, but we'll implement it as a no-op
    // for the server version since we handle game end differently
} 


// see http://stackoverflow.com/a/32008479/3155372
