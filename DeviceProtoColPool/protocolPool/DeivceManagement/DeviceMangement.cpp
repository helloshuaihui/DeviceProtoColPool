#include "DeviceMangement.h"
#include <fstream>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#endif

namespace DeviceMangement {

DeviceInit::DeviceInit(const std::string& configPath) 
    : m_configPath(configPath), 
      m_configLoaded(false), 
      m_dbInitialized(false),
      m_isTimedCollection(true),
      m_collectionInterval(1000),
      m_maxConnectionTime(1000),
      m_isReconnect(true),
      m_reconnectInterval(1000),
      m_ifAutoLoadDB(true) {
    
}

DeviceInit::~DeviceInit() {}

bool DeviceInit::fileExists(const std::string& path) const {
#ifdef _WIN32
    DWORD attr = GetFileAttributesA(path.c_str());
    return (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY));
#else
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0 && S_ISREG(buffer.st_mode));
#endif
}

bool DeviceInit::createDefaultConfig() {
    cJSON* root = cJSON_CreateObject();
    if (!root) return false;

    cJSON_AddStringToObject(root, "DBPath", "./DB/Device.db");
    cJSON_AddBoolToObject(root, "isTimedCollection", true);
    cJSON_AddNumberToObject(root, "CollectionInterval", 1000);
    cJSON_AddNumberToObject(root, "MaxConnectionTime", 1000);
    cJSON_AddBoolToObject(root, "isReconnect", true);
    cJSON_AddNumberToObject(root, "ReconnectInterval", 1000);
    cJSON_AddBoolToObject(root, "ifAutoLoadDB", true);

    char* jsonStr = cJSON_Print(root);
    if (!jsonStr) {
        cJSON_Delete(root);
        return false;
    }

    std::ofstream file(m_configPath);
    if (!file.is_open()) {
        cJSON_free(jsonStr);
        cJSON_Delete(root);
        return false;
    }

    file << jsonStr;
    file.close();

    cJSON_free(jsonStr);
    cJSON_Delete(root);
    return true;
}

bool DeviceInit::parseConfig(cJSON* root) {
    cJSON* item = nullptr;

    item = cJSON_GetObjectItem(root, "DBPath");
    if (item && cJSON_IsString(item)) {
        m_dbPath = item->valuestring;
    }

    item = cJSON_GetObjectItem(root, "isTimedCollection");
    if (item && cJSON_IsBool(item)) {
        m_isTimedCollection = item->valueint != 0;
    }

    item = cJSON_GetObjectItem(root, "CollectionInterval");
    if (item && cJSON_IsNumber(item)) {
        m_collectionInterval = item->valueint;
    }

    item = cJSON_GetObjectItem(root, "MaxConnectionTime");
    if (item && cJSON_IsNumber(item)) {
        m_maxConnectionTime = item->valueint;
    }

    item = cJSON_GetObjectItem(root, "isReconnect");
    if (item && cJSON_IsBool(item)) {
        m_isReconnect = item->valueint != 0;
    }

    item = cJSON_GetObjectItem(root, "ReconnectInterval");
    if (item && cJSON_IsNumber(item)) {
        m_reconnectInterval = item->valueint;
    }

    item = cJSON_GetObjectItem(root, "ifAutoLoadDB");
    if (item && cJSON_IsBool(item)) {
        m_ifAutoLoadDB = item->valueint != 0;
    }

    return true;
}

bool DeviceInit::loadConfig() {
    if (fileExists(m_configPath)) {
        std::ifstream file(m_configPath);
        if (!file.is_open()) {
            return false;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string jsonStr = buffer.str();
        file.close();

        cJSON* root = cJSON_Parse(jsonStr.c_str());
        if (!root) {
            return false;
        }

        bool result = parseConfig(root);
        cJSON_Delete(root);
        
        if (result) {
            m_configLoaded = true;
        }
        return result;
    } else {
        if (createDefaultConfig()) {
            return loadConfig();
        }
        return false;
    }
}

bool DeviceInit::createTables(DB::SQLite& db) {
    bool success = true;
    const char* protocolTypeTable = 
        "CREATE TABLE IF NOT EXISTS protocol_type ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "protocol TEXT NOT NULL UNIQUE, "
        "protocol_name TEXT NOT NULL, "
        "description TEXT)";
    success &= db.execute(protocolTypeTable);

    const char* protocolIndex = "CREATE INDEX IF NOT EXISTS idx_protocol ON protocol_type(protocol)";
    success &= db.execute(protocolIndex);

    const char* devicesTable = 
        "CREATE TABLE IF NOT EXISTS devices ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "pid INTEGER NOT NULL, "
        "protocol TEXT NOT NULL, "
        "device_name TEXT, "
        "description TEXT, "
        "status INTEGER DEFAULT 1, "
        "create_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
        "update_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP)";
    success &= db.execute(devicesTable);

    const char* devicesIndex1 = "CREATE INDEX IF NOT EXISTS idx_devices_pid ON devices(pid)";
    success &= db.execute(devicesIndex1);

    const char* devicesIndex2 = "CREATE INDEX IF NOT EXISTS idx_devices_protocol ON devices(protocol)";
    success &= db.execute(devicesIndex2);

    const char* collectionFieldTable = 
        "CREATE TABLE IF NOT EXISTS collection_field ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "did INTEGER NOT NULL, "
        "field_name TEXT NOT NULL, "
        "field_address TEXT NOT NULL, "
        "field_value_type TEXT NOT NULL, "
        "read_only INTEGER NOT NULL, "
        "endianness TEXT, "
        "description TEXT, "
        "create_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
        "update_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP)";
    success &= db.execute(collectionFieldTable);

    const char* fieldIndex = "CREATE INDEX IF NOT EXISTS idx_field_did ON collection_field(did)";
    success &= db.execute(fieldIndex);

    return success;
}

bool DeviceInit::loadDatabase() {
    if (!m_configLoaded && !loadConfig()) {
        return false;
    }
    DB::SQLite db(m_dbPath);
    bool dbExists = db.fileExists(m_dbPath);
    if (!db.open()) {
        if (!dbExists) {
            if (!createTables(db)) {
                db.close();
                return false;
            }
        }
        else {
            if (!db.tableExists("protocol_type") ||
                !db.tableExists("devices") ||
                !db.tableExists("collection_field")) {
                if (!createTables(db)) {
                    db.close();
                    return false;
                }
            }
        }
    }
    else {
        if (!db.tableExists("protocol_type") ||
            !db.tableExists("devices") ||
            !db.tableExists("collection_field")) {
            if (!createTables(db)) {
                db.close();
                return false;
            }
        }
    }
    m_dbInitialized = true;
    return true;
}

bool DeviceInit::initDatabase() {

    if (!m_configLoaded && !loadConfig()) {
        return false;
    }

    DB::SQLite db(m_dbPath);
    bool dbExists = db.fileExists(m_dbPath);

    if (!db.open()) {

        return false;
    }

    if (!dbExists && m_ifAutoLoadDB) {
        if (!createTables(db)) {
            db.close();
            return false;
        }
    }

    db.close();
    m_dbInitialized = true;
    return true;
}

bool DeviceInit::isConfigLoaded() const {
    return m_configLoaded;
}

bool DeviceInit::isDatabaseInitialized() const {
    return m_dbInitialized;
}

std::string DeviceInit::getDBPath() const {
    return m_dbPath;
}

bool DeviceInit::getIsTimedCollection() const {
    return m_isTimedCollection;
}

int DeviceInit::getCollectionInterval() const {
    return m_collectionInterval;
}

int DeviceInit::getMaxConnectionTime() const {
    return m_maxConnectionTime;
}

bool DeviceInit::getIsReconnect() const {
    return m_isReconnect;
}

int DeviceInit::getReconnectInterval() const {
    return m_reconnectInterval;
}

bool DeviceInit::getIfAutoLoadDB() const {
    return m_ifAutoLoadDB;
}

DeviceCollect::DeviceCollect() {}

DeviceCollect::~DeviceCollect() {}

} // namespace DeviceMangement