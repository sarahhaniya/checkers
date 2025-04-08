// DatabaseManager.h
#ifndef DATABASE_MANAGER_H
#define DATABASE_MANAGER_H

#include "sqlite3.h"
#include <string>
#include <vector>

class DatabaseManager {
public:
    DatabaseManager();
    ~DatabaseManager();
    
    bool initialize(const std::string& dbPath = "checkers_game.db");
    bool createUser(const std::string& username, const std::string& email, const std::string& passwordHash);    
    bool validateUser(const std::string& username, const std::string& passwordHash);
    bool userExists(const std::string& username, const std::string& email);
    bool incrementWins(const std::string& username);
    bool incrementLosses(const std::string& username);
    int getWins(const std::string& username);
    int getLosses(const std::string& username);
    
    // int createGameSession(const std::string& player1);
    // bool updateGameState(int sessionId, const std::string& boardState);
    // std::string getGameState(int sessionId);
    // bool recordMove(int sessionId, int fromX, int fromY, int toX, int toY, bool isWhite);
    
private:
    sqlite3* db;  
    bool isConnected;
};

#endif 