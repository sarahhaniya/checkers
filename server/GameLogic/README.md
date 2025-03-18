# Checkers (C++)

**Date:** 5.30.2016

**Author(s):** Mckenna Cisler
https://github.com/MckennaCisler/checkers-cpp

## PURPOSE OF PROJECT
Build a 2-player checkers game.

## DEPENDENCIES
g++ compiler, c/c++ standard library

## HOW TO RUN THIS PROJECT
Run `make` to compile (optionally run `make clean` before), then run the main program checkers using `./checkers`

## CLASS SUMMARY
### HumanPlayer
Responsible for interacting with a human player in order to determine their move and apply it to the board.

Note: Moves are identified by displaying possible ones to the user and having them choose from that list.

### Board
Stores and allows manipulation of the game board and game pieces.

### Piece
Responsible for storing data associated with a certain piece and determining properties of that piece such as available moves.

### Move
Stores data associated with the move of a piece, and methods to determine further properties.

### Typedef.h
Stores a few type definitions needed in certain aspects of the program.

#### Player (Abstract)
Responsible for outlining shared methods of the HumanPlayer classes so they can be used interchangeably.