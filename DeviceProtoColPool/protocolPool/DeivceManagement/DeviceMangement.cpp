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
    db = new DB::SQLite();
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

bool DeviceInit::createFile(const std::string& filePath)
{
    size_t pos = filePath.find_last_of("/\\");
    if (pos != std::string::npos) {
        std::string dir = filePath.substr(0, pos);

        struct stat info {};
        if (!(stat(dir.c_str(), &info) == 0 && (info.st_mode & S_IFDIR))) {
            // 创建目录
            #ifdef _WIN32
                mkdir(dir.c_str());
            #else
                mkdir(dir.c_str(), 0755);
            #endif
        }
    }

    // 2. 创建文件
    std::ofstream file(filePath);
    bool ok = file.is_open();
    file.close();
    return ok;
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
bool DeviceInit::loadDatabase() {
    if (!m_configLoaded && !loadConfig()) {
        return false;
    }
    db->m_dbPath = m_dbPath;
    bool dbExists = db->fileExists(m_dbPath);
    if (!dbExists) {
        createFile(m_dbPath);
    }
    bool isOpen = db->open();
    if (isOpen) {
        if (!db->tableExists("protocol_type")) {
            const char* protocolTypeTable =
                "CREATE TABLE IF NOT EXISTS protocol_type ("
                "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                "protocol TEXT NOT NULL UNIQUE, "
                "protocol_name TEXT NOT NULL, "
                "description TEXT)";
            std::cout << "protocol_type表初始化：" << (db->execute(protocolTypeTable) ? "成功" : "失败") << std::endl;

            const char* protocolIndex = "CREATE INDEX IF NOT EXISTS idx_protocol ON protocol_type(protocol)";
            db->execute(protocolIndex);
        }
        if (!db->tableExists("devices")) {
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
            std::cout << "devices表初始化：" << (db->execute(devicesTable)?"成功":"失败" )<< std::endl;

            const char* devicesIndex1 = "CREATE INDEX IF NOT EXISTS idx_devices_pid ON devices(pid)";
            db->execute(devicesIndex1);
        }
        if (!db->tableExists("collection_field")) {
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
            std::cout << "devices表初始化：" << (db->execute(collectionFieldTable) ? "成功" : "失败") << std::endl;

            const char* fieldIndex = "CREATE INDEX IF NOT EXISTS idx_field_did ON collection_field(did)";
            db->execute(fieldIndex);
        }
    }
    else {
        db->close();
        return false;
    }
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

DeviceCollect::DeviceCollect() {
    //加载配置 数据库
    this->loadConfig();
    this->loadDatabase();
    this->LoadDeviceData();
}

DeviceCollect::~DeviceCollect() {}
bool DeviceCollect::AddProtocolType(const ProtocolType& protocolType) {
    if (!db || !db->isOpen()) {
        return false;
    }
    std::map<std::string, std::string> data;
    data["protocol"] = protocolType.protocol;
    data["protocol_name"] = protocolType.protocolName;
    data["description"] = protocolType.description;
    return db->insert("protocol_type", data);
}

bool DeviceCollect::DeleteProtocolType(int id) {
    if (!db || !db->isOpen()) {
        return false;
    }
    std::string whereClause = "id = " + std::to_string(id);
    return db->remove("protocol_type", whereClause);
}
bool DeviceCollect::AddDevice(const Device& device) {
    if (!db || !db->isOpen()) {
        return false;
    }
    std::map<std::string, std::string> data;
    data["pid"] = std::to_string(device.pid);
    data["protocol"] = device.protocol;
    data["device_name"] = device.deviceName;
    data["description"] = device.description;
    data["status"] = std::to_string(device.status);
    return db->insert("devices", data);
}

bool DeviceCollect::DeleteDevice(int id) {
    if (!db || !db->isOpen()) {
        return false;
    }
    std::string whereClause = "id = " + std::to_string(id);
    return db->remove("devices", whereClause);
}

bool DeviceCollect::DeleteDevicesByProtocolId(int pid) {
    if (!db || !db->isOpen()) {
        return false;
    }
    std::string whereClause = "pid = " + std::to_string(pid);
    return db->remove("devices", whereClause);
}
bool DeviceCollect::AddCollectionField(const CollectionField& field) {
    if (!db || !db->isOpen()) {
        return false;
    }
    std::map<std::string, std::string> data;
    data["did"] = std::to_string(field.did);
    data["field_name"] = field.fieldName;
    data["field_address"] = field.fieldAddress;
    data["field_value_type"] = field.fieldValueType;
    data["read_only"] = std::to_string(field.readOnly);
    data["endianness"] = field.endianness;
    data["description"] = field.description;
    return db->insert("collection_field", data);
}

bool DeviceCollect::DeleteCollectionField(int id) {
    if (!db || !db->isOpen()) {
        return false;
    }
    std::string whereClause = "id = " + std::to_string(id);
    return db->remove("collection_field", whereClause);
}

bool DeviceCollect::DeleteCollectionFieldsByDeviceId(int did) {
    if (!db || !db->isOpen()) {
        return false;
    }
    std::string whereClause = "did = " + std::to_string(did);
    return db->remove("collection_field", whereClause);
}

bool DeviceCollect::LoadDeviceData()
{
    auto dbData = db->query("select * from protocol_type");
    for (const auto& row : dbData) {
        ProtocolType protocolType = {0};
        for (const auto& pair : row) {
            std::string field = pair.first;
            std::string value = pair.second;
            if (field.compare("id") == 0) {
                protocolType.id = std::atoi(value.c_str());
            }
            if (field.compare("protocol") == 0) {
                protocolType.protocol = value;
            }
            if (field.compare("protocol_name") == 0) {
                protocolType.protocolName = value;
            }
            if (field.compare("description") == 0) {
                protocolType.description = value;
            }
        }
        this->ProtocolTypes.push_back(protocolType);
    }
    dbData = db->query("select * from devices");
    for (const auto& row : dbData) {
        Device device = { 0 };
        for (const auto& pair : row) {
            std::string field = pair.first;
            std::string value = pair.second;
            if (field.compare("id") == 0) {
                device.id = std::atoi(value.c_str());
            }
            if (field.compare("pid") == 0) {
                device.pid = std::atoi(value.c_str());
            }
            if (field.compare("protocol") == 0) {
                device.protocol = value;
            }
            if (field.compare("device_name") == 0) {
                device.deviceName = value;
            }
            if (field.compare("description") == 0) {
                device.description = value;
            }
            if (field.compare("status") == 0) {
                device.status = std::atoi(value.c_str());
            }
        }
        this->Devices.push_back(device);
    }
    dbData = db->query("select * from CollectionField");
    for (const auto& row : dbData) {
        CollectionField collectionField = { 0 };
        for (const auto& pair : row) {
            std::string field = pair.first;
            std::string value = pair.second;
            if (field.compare("id") == 0) {
                collectionField.id = std::atoi(value.c_str());
            }
            if (field.compare("did") == 0) {
                collectionField.did = std::atoi(value.c_str());
            }
            if (field.compare("field_name") == 0) {
                collectionField.fieldName = value;
            }
            if (field.compare("field_address") == 0) {
                collectionField.fieldAddress = value;
            }
            if (field.compare("field_value_type") == 0) {
                collectionField.fieldValueType = value;
            }
            if (field.compare("read_only") == 0) {
                collectionField.readOnly = std::atoi(value.c_str());
            }
            if (field.compare("endianness") == 0) {
                collectionField.endianness = value;
            }
            if (field.compare("description") == 0) {
                collectionField.description = value;
            }
        }
        Device& device = GetDeviceInfo(collectionField.did);
        if (device.id != 0) {
            device.CollectionFields.push_back(collectionField);
        }
    }
    return true;
}

Device& DeviceCollect::GetDeviceInfo(int id)
{
    for (auto device : this->Devices) {
        if (device.id = id) {
            return device;
        }
    }
    return Device();
}

} // namespace DeviceMangement