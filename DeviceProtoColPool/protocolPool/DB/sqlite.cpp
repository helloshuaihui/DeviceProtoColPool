#include "sqlite.h"
#include <fstream>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#endif

namespace protocolPool {
namespace DB {

SQLite::SQLite(const std::string& dbPath) 
    : m_dbPath(dbPath), m_db(nullptr), m_lastError(0) {}

SQLite::~SQLite() {
    close();
}

bool SQLite::fileExists(const std::string& path) const {
#ifdef _WIN32
    DWORD attr = GetFileAttributesA(path.c_str());
    return (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY));
#else
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0 && S_ISREG(buffer.st_mode));
#endif
}

void SQLite::setLastError(int code, const std::string& message) {
    m_lastError = code;
    m_lastErrorMessage = message;
}

bool SQLite::open() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_db != nullptr) {
        close();
    }
    
    bool dbExists = fileExists(m_dbPath);
    
    int rc = sqlite3_open(m_dbPath.c_str(), &m_db);
    if (rc != SQLITE_OK) {
        setLastError(rc, sqlite3_errmsg(m_db));
        sqlite3_close(m_db);
        m_db = nullptr;
        return false;
    }
    
    if (!dbExists) {
    }
    
    setLastError(SQLITE_OK, "");
    return true;
}

void SQLite::close() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_db != nullptr) {
        sqlite3_close(m_db);
        m_db = nullptr;
    }
}

bool SQLite::isOpen() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_db != nullptr;
}

bool SQLite::execute(const std::string& sql) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_db == nullptr) {
        setLastError(-1, "Database not open");
        return false;
    }
    
    char* errMsg = nullptr;
    int rc = sqlite3_exec(m_db, sql.c_str(), nullptr, nullptr, &errMsg);
    
    if (rc != SQLITE_OK) {
        setLastError(rc, errMsg ? errMsg : "Unknown error");
        sqlite3_free(errMsg);
        return false;
    }
    
    sqlite3_free(errMsg);
    setLastError(SQLITE_OK, "");
    return true;
}

bool SQLite::insert(const std::string& tableName, const std::map<std::string, std::string>& data) {
    if (data.empty()) {
        setLastError(-1, "No data to insert");
        return false;
    }
    
    std::stringstream ss;
    ss << "INSERT INTO " << tableName << " (";
    
    std::string columns;
    std::string values;
    
    for (auto it = data.begin(); it != data.end(); ++it) {
        if (it != data.begin()) {
            columns += ", ";
            values += ", ";
        }
        columns += it->first;
        values += "'" + it->second + "'";
    }
    
    ss << columns << ") VALUES (" << values << ")";
    
    return execute(ss.str());
}

bool SQLite::update(const std::string& tableName, const std::map<std::string, std::string>& data, const std::string& whereClause) {
    if (data.empty()) {
        setLastError(-1, "No data to update");
        return false;
    }
    
    std::stringstream ss;
    ss << "UPDATE " << tableName << " SET ";
    
    for (auto it = data.begin(); it != data.end(); ++it) {
        if (it != data.begin()) {
            ss << ", ";
        }
        ss << it->first << " = '" << it->second << "'";
    }
    
    if (!whereClause.empty()) {
        ss << " WHERE " << whereClause;
    }
    
    return execute(ss.str());
}

bool SQLite::remove(const std::string& tableName, const std::string& whereClause) {
    std::stringstream ss;
    ss << "DELETE FROM " << tableName;
    
    if (!whereClause.empty()) {
        ss << " WHERE " << whereClause;
    }
    
    return execute(ss.str());
}

int SQLite::queryCallback(void* data, int argc, char** argv, char** azColName) {
    auto* result = static_cast<std::vector<std::map<std::string, std::string>>*>(data);
    
    std::map<std::string, std::string> row;
    for (int i = 0; i < argc; ++i) {
        row[azColName[i]] = argv[i] ? argv[i] : "";
    }
    
    result->push_back(row);
    return 0;
}

std::vector<std::map<std::string, std::string>> SQLite::query(const std::string& sql) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::vector<std::map<std::string, std::string>> result;
    
    if (m_db == nullptr) {
        setLastError(-1, "Database not open");
        return result;
    }
    
    char* errMsg = nullptr;
    int rc = sqlite3_exec(m_db, sql.c_str(), queryCallback, &result, &errMsg);
    
    if (rc != SQLITE_OK) {
        setLastError(rc, errMsg ? errMsg : "Unknown error");
        sqlite3_free(errMsg);
        return result;
    }
    
    sqlite3_free(errMsg);
    setLastError(SQLITE_OK, "");
    return result;
}

bool SQLite::tableExists(const std::string& tableName) {
    std::stringstream ss;
    ss << "SELECT name FROM sqlite_master WHERE type='table' AND name='" << tableName << "'";
    
    auto result = query(ss.str());
    return !result.empty();
}

bool SQLite::createTable(const std::string& tableName, const std::string& columns) {
    std::stringstream ss;
    ss << "CREATE TABLE IF NOT EXISTS " << tableName << " (" << columns << ")";
    
    return execute(ss.str());
}

bool SQLite::reconnect() {
    close();
    return open();
}

int SQLite::getLastError() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_lastError;
}

std::string SQLite::getLastErrorMessage() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_lastErrorMessage;
}

} // namespace DB
} // namespace protocolPool