#include "DatabaseManager.h"
#include "sqlite3.h"
#include <iostream>
#include <functional>
#include <sstream>
#include <iomanip>

DatabaseManager::DatabaseManager() : db(nullptr), isConnected(false) {}

int DatabaseManager::getWins(const std::string& username) {
    std::string query = "SELECT wins FROM users WHERE username = ?";
    sqlite3_stmt* stmt;
    int wins = 0;
    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            wins = sqlite3_column_int(stmt, 0);
        }
    }
    sqlite3_finalize(stmt);
    return wins;
}

int DatabaseManager::getLosses(const std::string& username) {
    std::string query = "SELECT losses FROM users WHERE username = ?";
    sqlite3_stmt* stmt;
    int losses = 0;
    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            losses = sqlite3_column_int(stmt, 0);
        }
    }
    sqlite3_finalize(stmt);
    return losses;
}

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

bool DatabaseManager::userExists(const std::string& username, const std::string& email) {
    if (!db) return false;

    const char* query = "SELECT COUNT(*) FROM users WHERE username = ? OR email = ?";
    sqlite3_stmt* stmt;
    bool exists = false;

    if (sqlite3_prepare_v2((sqlite3*)db, query, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, email.c_str(), -1, SQLITE_STATIC);

        if (sqlite3_step(stmt) == SQLITE_ROW) {
            int count = sqlite3_column_int(stmt, 0);
            exists = (count > 0);
        }

        sqlite3_finalize(stmt);
    }

    return exists;
}

bool DatabaseManager::incrementWins(const std::string& username) {
    std::string query = "UPDATE users SET wins = wins + 1 WHERE username = ?";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
}

bool DatabaseManager::incrementLosses(const std::string& username) {
    std::string query = "UPDATE users SET losses = losses + 1 WHERE username = ?";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
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