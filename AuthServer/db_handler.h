#pragma once

#include <map>

#include "mysqlutil.h"
#include "message.h"

enum class StatementType {
    kCREATE_USER_WEB_AUTH = 0,        // create user in web_auth table (create user, store email + password)
    kREAD_USER_WEB_AUTH = 1,          // read user from web_auth table (find user by email)
    kCREATE_USER_USER = 2,            // create user in user table
    kREAD_USER_MAX_ID_USER = 3,       // read user max id from user table
    kUPDATE_USER_LAST_LOGIN_UER = 4,  // update user in user table (update last_login)
};

class DBHandler {
public:
    DBHandler();
    ~DBHandler();

    int Initialize(const char* host, const char* username, const char* password, const char* schema);

    //--------------------
    // Service Logic
    //--------------------
    network::CreateAccountFailureReason CreateAccount(const std::string& email, const std::string& plaintextPassword,
                                                      uint64_t& outUserId);
    network::AuthenticateAccountFailureReason AuthenticateAccount(const std::string& email,
                                                                  const std::string& plaintextPassword,
                                                                  uint64_t& outUserId);

private:
    //--------------------
    // Prepared Statements - CRUD operations
    // NOTE: we should keep these operations at a low level, and not tangled with the service logic
    //--------------------
    int CreateUserInWebAuth(const std::string& email, const std::string& salt, const std::string& hashedPassword,
                            const sql::SQLString& userId);
    bool ReadUserInWebAuth(const std::string& email, std::string& outSalt, std::string& outHashedPassword,
                           uint64_t& outUserId);
    int CreateUserInUser(const sql::SQLString& lastLogin, const sql::SQLString& creationDate);
    uint64_t ReadUserMaxIdInUser();
    int UpdateUserLastLoginInUser(const sql::SQLString& userId, uint64_t lastLogin);

    //-----------------------------
    // Helpers
    //-----------------------------
    std::string GenerateRandomString(size_t length);

private:
    MySQLUtil g_MySQLDB;
    std::map<int, sql::PreparedStatement*> g_PreparedStatements;
};