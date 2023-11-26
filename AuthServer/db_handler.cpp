#include "db_handler.h"

#include <chrono>
#include <iomanip>
#include <random>
#include <sstream>

#include "bcrypt.h"

DBHandler::DBHandler() {}

DBHandler::~DBHandler() {}

int DBHandler::Initialize(const char* host, const char* username, const char* password, const char* schema) {
    // Initialize MySQL driver and connect to our database
    //"127.0.0.1:3306", "root", "root", "chat"
    g_MySQLDB.ConnectToDatabase(host, username, password, schema);

    if (!g_MySQLDB.IsConnected()) {
        return -1;
    }

    //  Prepare our statements
    g_PreparedStatements[(int)StatementType::kCREATE_USER_WEB_AUTH] =
        g_MySQLDB.PrepareStatement("INSERT INTO web_auth (email, salt, hashed_password, user_id) VALUES (?, ?, ?, ?);");

    g_PreparedStatements[(int)StatementType::kREAD_USER_WEB_AUTH] =
        g_MySQLDB.PrepareStatement("SELECT salt, hashed_password, user_id FROM web_auth WHERE email = ?");

    g_PreparedStatements[(int)StatementType::kCREATE_USER_USER] =
        g_MySQLDB.PrepareStatement("INSERT INTO user (last_login, creation_date) VALUES (?, ?);");

    g_PreparedStatements[(int)StatementType::kREAD_USER_MAX_ID_USER] =
        g_MySQLDB.PrepareStatement("SELECT MAX(id) AS max_id FROM user;");

    g_PreparedStatements[(int)StatementType::kUPDATE_USER_LAST_LOGIN_UER] =
        g_MySQLDB.PrepareStatement("UPDATE user SET last_login = ? WHERE id = ?");

    return 0;
}

/**
 * Creates a user in the web authentication table.
 *
 * @param email the email of the user
 * @param salt the salt value
 * @param hashedPassword the hashed password
 * @param userId the user ID
 *
 * @return the number of rows affected
 */
int DBHandler::CreateUserInWebAuth(const std::string& email, const std::string& salt, const std::string& hashedPassword,
                                   const sql::SQLString& userId) {
    sql::PreparedStatement* pStmt = g_PreparedStatements[(int)StatementType::kCREATE_USER_WEB_AUTH];
    pStmt->setString(1, email);
    pStmt->setString(2, salt);
    pStmt->setString(3, hashedPassword);
    pStmt->setBigInt(4, userId);

    return pStmt->executeUpdate();
}

/**
 * Reads user information from the web authentication table.
 *
 * @param email the email of the user
 * @param outSalt the output variable to store the salt value
 * @param outHashedPassword the output variable to store the hashed password
 * @param outUserId the output variable to store the user ID
 *
 * @return true if the user is found in the table, false otherwise
 *
 * @throws std::exception if an error occurs while accessing the database
 */
bool DBHandler::ReadUserInWebAuth(const std::string& email, std::string& outSalt, std::string& outHashedPassword,
                                  uint64_t& outUserId) {
    sql::PreparedStatement* pStmt = g_PreparedStatements[(int)StatementType::kREAD_USER_WEB_AUTH];
    pStmt->setString(1, email);

    sql::ResultSet* result = pStmt->executeQuery();
    bool found = false;
    while (result->next()) {
        try {
            outSalt = result->getString("salt");
            outHashedPassword = result->getString("hashed_password");
            outUserId = result->getUInt64("user_id");
        } catch (const std::exception& ex) {
            std::cerr << ex.what() << std::endl;
        }
        found = true;
    }
    return found;
}

/**
 * Creates a user in the "user" table.
 *
 * @param lastLogin The last login date and time of the user.
 * @param creationDate The creation date and time of the user.
 *
 * @return The number of rows affected in the "user" table.
 */
int DBHandler::CreateUserInUser(const sql::SQLString& lastLogin, const sql::SQLString& creationDate) {
    sql::PreparedStatement* pStmt = g_PreparedStatements[(int)StatementType::kCREATE_USER_USER];
    pStmt->setDateTime(1, lastLogin);
    pStmt->setDateTime(2, creationDate);

    return pStmt->executeUpdate();
}

/**
 * Reads the maximum user ID from the "user" table in the database.
 *
 * @return The maximum user ID found in the "user" table.
 *
 * @throws std::exception If an error occurs while executing the SQL query or retrieving the result.
 */
uint64_t DBHandler::ReadUserMaxIdInUser() {
    sql::PreparedStatement* pStmt = g_PreparedStatements[(int)StatementType::kREAD_USER_MAX_ID_USER];
    sql::ResultSet* result = pStmt->executeQuery();
    uint16_t id = 0;
    if (result->next()) {
        try {
            id = result->getInt("max_id");
        } catch (const std::exception& ex) {
            std::cerr << ex.what() << std::endl;
        }
    }
    return id;
}

/**
 * Updates the last login timestamp for a user in the User table.
 *
 * @param userId the ID of the user to update
 * @param lastLogin the new last login timestamp
 *
 * @return the number of rows affected by the update statement
 */
int DBHandler::UpdateUserLastLoginInUser(const sql::SQLString& userId, uint64_t lastLogin) {
    sql::PreparedStatement* pStmt = g_PreparedStatements[(int)StatementType::kUPDATE_USER_LAST_LOGIN_UER];
    pStmt->setUInt64(1, lastLogin);
    pStmt->setBigInt(2, userId);

    return pStmt->executeUpdate();
}

/**
 * Generates a random string of the specified length.
 *
 * @param length the length of the random string
 *
 * @return the generated random string
 */
std::string DBHandler::GenerateRandomString(size_t length) {
    const char charset[] =
        "0123456789"
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    const size_t max_index = (sizeof(charset) - 1);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distr(0, max_index - 1);

    std::string str;
    for (size_t i = 0; i < length; ++i) {
        str += charset[distr(gen)];
    }
    return str;
}

/**
 * Creates a new user account with the provided email and password.
 *
 * @param email The email address of the user.
 * @param plaintextPassword The plain text password of the user.
 *
 * @return The result of creating the user account. Possible values are:
 *         - kSUCCESS: Account creation was successful.
 *         - kINVALID_PASSWORD: The provided password is invalid.
 *         - kACCOUNT_ALREADY_EXISTS: An account with the provided email already exists.
 *         - kINTERNAL_SERVER_ERROR: An internal server error occurred.
 */
network::CreateAccountFailureReason DBHandler::CreateAccount(const std::string& email,
                                                             const std::string& plaintextPassword,
                                                             uint64_t& outUserId) {
    // 1. Check if password is valid
    if (plaintextPassword.length() < 8) {
        return network::CreateAccountFailureReason::kINVALID_PASSWORD;
    }
    // 2. Search if user exists in web_auth table
    std::string salt{""};
    std::string hashedPassword{""};
    bool userExists = ReadUserInWebAuth(email, salt, hashedPassword, outUserId);
    // 3. If user exists, return false
    if (userExists) {
        return network::CreateAccountFailureReason::kACCOUNT_ALREADY_EXISTS;
    }
    // 4. If user does not exist, create user in user table then in web_auth table
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    time_t timestamp = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    std::tm time_struct = {};
    localtime_s(&time_struct, &timestamp);
    ss << std::put_time(&time_struct, "%Y-%m-%d %H:%M:%S");

    sql::SQLString datetime{ss.str()};

    int result = CreateUserInUser(datetime, datetime);
    if (result > 0) {
        outUserId = ReadUserMaxIdInUser();
        if (outUserId == 0) {
            return network::CreateAccountFailureReason::kINTERNAL_SERVER_ERROR;
        }
        // add salt to password
        salt = GenerateRandomString(4);
        hashedPassword = bcrypt::generateHash(plaintextPassword + salt);

        result = CreateUserInWebAuth(email.c_str(), salt.c_str(), hashedPassword.c_str(), std::to_string(outUserId));
    }
    return network::CreateAccountFailureReason::kSUCCESS;
}

/**
 * Authenticates a user with the provided email and password.
 *
 * @param email The email address of the user.
 * @param plaintextPassword The plain text password of the user.
 *
 * @return The result of authenticating the user. Possible values are:
 *         - kSUCCESS: Authentication was successful.
 *         - kINVALID_CREDENTIALS: Invalid email or password.
 *         - kINTERNAL_SERVER_ERROR: An internal server error occurred.
 */
network::AuthenticateAccountFailureReason DBHandler::AuthenticateAccount(const std::string& email,
                                                                         const std::string& plaintextPassword,
                                                                         uint64_t& outUserId) {
    // 1. Check if password is valid
    if (plaintextPassword.length() < 8) {
        return network::AuthenticateAccountFailureReason::kINVALID_CREDENTIALS;
    }
    // 2. Search if user exists in web_auth table
    std::string salt{""};
    std::string hashedPassword{""};
    bool userExists = ReadUserInWebAuth(email, salt, hashedPassword, outUserId);
    // 3. If user does not exist, return false
    if (!userExists) {
        return network::AuthenticateAccountFailureReason::kINVALID_CREDENTIALS;
    }
    // 4. If user exists, compare password
    bool valid = bcrypt::validatePassword(plaintextPassword + salt, hashedPassword);
    if (!valid) {
        return network::AuthenticateAccountFailureReason::kINVALID_CREDENTIALS;
    }

    return network::AuthenticateAccountFailureReason::kSUCCESS;
}
