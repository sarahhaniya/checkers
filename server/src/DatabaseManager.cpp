#include "DatabaseManager.h"
#include "sqlite3.h"
#include <iostream>
#include <functional>
#include <sstream>
#include <iomanip>

DatabaseManager::DatabaseManager() : db(nullptr), isConnected(false) {}

DatabaseManager::~DatabaseManager() {
    if (db) {
        sqlite3_close((sqlite3*)db);
        db = nullptr;
    }
    isConnected = false;
}

bool DatabaseManager::initialize(const std::string& dbPath) {
    std::cout << "Opening database file: " << dbPath << std::endl;
    int rc = sqlite3_open(dbPath.c_str(), (sqlite3**)&db);
    std::cout << "sqlite3_open result: " << rc << std::endl;
    if (rc) {
        std::cerr << "CRITICAL: Cannot open database: " << sqlite3_errmsg((sqlite3*)db) << std::endl;
        return false;
    }

    const char* createUsersSql = 
        "CREATE TABLE IF NOT EXISTS users("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "username TEXT UNIQUE, "
        "email TEXT, "
        "password_hash TEXT, "
        "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP);";

    char* errMsg = nullptr;
    rc = sqlite3_exec((sqlite3*)db, createUsersSql, nullptr, nullptr, &errMsg);
    std::cout << "sqlite3_exec result: " << rc << ", error message: " << (errMsg ? errMsg : "None") << std::endl;
    if (rc != SQLITE_OK) {
        std::cerr << "CRITICAL: SQL error creating users table: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }

    std::cout << "Database and users table initialized successfully" << std::endl;
    return true;
}

bool DatabaseManager::createUser(const std::string& username, const std::string& email, const std::string& passwordHash) {
    char* errMsg = nullptr;
    std::string sql = "INSERT INTO users (username, email, password_hash) VALUES ('" + 
                      username + "', '" + email + "', '" + passwordHash + "');";
    
    int rc = sqlite3_exec((sqlite3*)db, sql.c_str(), nullptr, nullptr, &errMsg);
    std::cout << "createUser result: " << rc << ", error message: " << (errMsg ? errMsg : "None") << std::endl;

    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

bool DatabaseManager::validateUser(const std::string& username, const std::string& passwordHash) {
    sqlite3_stmt* stmt;
    std::string sql = "SELECT COUNT(*) FROM users WHERE username = ? AND password_hash = ?;";
    
    if (sqlite3_prepare_v2((sqlite3*)db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, passwordHash.c_str(), -1, SQLITE_STATIC);
    
    bool valid = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        valid = (sqlite3_column_int(stmt, 0) > 0);
    }
    
    sqlite3_finalize(stmt);
    return valid;
}


bool DatabaseManager::updateGameState(int sessionId, const std::string& boardState) {
    char* errMsg = nullptr;
    sqlite3_stmt* stmt;
    std::string checkSql = "SELECT COUNT(*) FROM games WHERE session_id = ?;";
    
    if (sqlite3_prepare_v2((sqlite3*)db, checkSql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    
    sqlite3_bind_int(stmt, 1, sessionId);
    
    bool exists = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        exists = (sqlite3_column_int(stmt, 0) > 0);
    }
    
    sqlite3_finalize(stmt);
    
    std::string sql;
    if (exists) {
        sql = "UPDATE games SET board_state = '" + boardState + 
              "', last_updated = CURRENT_TIMESTAMP WHERE session_id = " + 
              std::to_string(sessionId) + ";";
    } else {
        sql = "INSERT INTO games (session_id, board_state, status) VALUES (" + 
              std::to_string(sessionId) + ", '" + boardState + "', 'ACTIVE');";
    }
    
    int rc = sqlite3_exec((sqlite3*)db, sql.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

int DatabaseManager::createGameSession(const std::string& player1) {
    char* errMsg = nullptr;
    
    std::string sql = "INSERT INTO games (player1, status) VALUES ('" + 
                      player1 + "', 'WAITING');";
    
    int rc = sqlite3_exec((sqlite3*)db, sql.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error in createGameSession: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return -1;
    }
    
    sqlite3_stmt* stmt;
    std::string lastIdSql = "SELECT last_insert_rowid();";
    
    if (sqlite3_prepare_v2((sqlite3*)db, lastIdSql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return -1;
    }
    
    int sessionId = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        sessionId = sqlite3_column_int(stmt, 0);
    }
    
    sqlite3_finalize(stmt);
    return sessionId;
}

std::string DatabaseManager::getGameState(int sessionId) {
    sqlite3_stmt* stmt;
    std::string sql = "SELECT board_state FROM games WHERE session_id = ?;";
    
    if (sqlite3_prepare_v2((sqlite3*)db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement in getGameState: " << sqlite3_errmsg((sqlite3*)db) << std::endl;
        return "";
    }
    
    sqlite3_bind_int(stmt, 1, sessionId);
    
    std::string boardState;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char* text = sqlite3_column_text(stmt, 0);
        if (text) {
            boardState = reinterpret_cast<const char*>(text);
        }
    }
    
    sqlite3_finalize(stmt);
    return boardState;
}

bool DatabaseManager::recordMove(int sessionId, int fromX, int fromY, int toX, int toY, bool isWhite) {
    char* errMsg = nullptr;
    std::stringstream sql;
    sql << "INSERT INTO moves (session_id, from_x, from_y, to_x, to_y, is_white) VALUES ("
        << sessionId << ", "
        << fromX << ", "
        << fromY << ", "
        << toX << ", "
        << toY << ", "
        << (isWhite ? "1" : "0") << ");";
    
    int rc = sqlite3_exec((sqlite3*)db, sql.str().c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error in recordMove: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}
