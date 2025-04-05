// DatabaseManager.h
#ifndef DATABASE_MANAGER_H
#define DATABASE_MANAGER_H

#include <string>
#include <vector>

class DatabaseManager {
public:
    DatabaseManager();
    ~DatabaseManager();
    
    bool initialize(const std::string& dbPath = "checkers_game.db");
    bool createUser(const std::string& username, const std::string& email, const std::string& passwordHash);    bool validateUser(const std::string& username, const std::string& passwordHash);
    int createGameSession(const std::string& player1);
    bool updateGameState(int sessionId, const std::string& boardState);
    std::string getGameState(int sessionId);
    bool recordMove(int sessionId, int fromX, int fromY, int toX, int toY, bool isWhite);
    
private:
    void* db;  
    bool isConnected;
};

#endif 