#pragma once
#pragma execution_character_set("utf-8")
#include <cjson/cJSON.h>
#include <string>
#include <iostream>
#include <fstream>
#include "../DB/sqlite.h" 
#include "../ModbusTcp/ModbusTcp.h"
#ifdef _WIN32
    #include <windows.h>
    #include <direct.h> 
#else
    #include <unistd.h>
    #include <sys/stat.h>
#endif
namespace DeviceMangement {

class DeviceInit {
public:
	//构造函数
    DeviceInit(const std::string& configPath = "DeviceCollectConfig.json");
    //析构函数
    ~DeviceInit();
	//加载配置文件
    bool loadConfig();
	//加载数据库
    bool loadDatabase();
	//判断配置文件是否加载成功
    bool isConfigLoaded() const;
	//判断数据库是否初始化成功
    bool isDatabaseInitialized() const;
	//获取数据库路径
    std::string getDBPath() const;
	//获取是否定时采集
    bool getIsTimedCollection() const;
	//获取采集间隔
    int getCollectionInterval() const;
	//获取最大连接时间
    int getMaxConnectionTime() const;
	//获取是否自动重连
    bool getIsReconnect() const;
	//获取重连间隔
    int getReconnectInterval() const;
	//获取是否自动加载数据库
    bool getIfAutoLoadDB() const;
    //数据库连接
    DB::SQLite* db;
private:
	//配置文件路径
    std::string m_configPath;
	//配置文件是否加载成功
    bool m_configLoaded;
	//数据库是否初始化成功
    bool m_dbInitialized;
	//数据库路径
    std::string m_dbPath;
	//是否定时采集
    bool m_isTimedCollection;
	//采集间隔
    int m_collectionInterval;
	//最大连接时间
    int m_maxConnectionTime;
	//是否自动重连
    bool m_isReconnect;
	//重连间隔
    int m_reconnectInterval;
	//是否自动加载数据库
    bool m_ifAutoLoadDB;
	//创建默认配置文件
    bool createDefaultConfig();
	//解析配置文件
    bool parseConfig(cJSON* root);
	//判断文件是否存在
    bool fileExists(const std::string& path) const;
    //创建文件
    bool createFile(const std::string& filePath);
};
// 协议类型
struct ProtocolType {
    int32_t id = 0;                 // 协议ID
    std::string protocol;       // 协议类型
    std::string protocolName;   // 协议名称
    std::string description;    // 协议描述
};
// 采集字段
struct CollectionField {
    int32_t id = 0;                 // 字段ID
    int32_t did = 0;                // 关联设备ID
    std::string fieldName;      // 字段名
    std::string fieldAddress;   // 寄存器地址
    std::string fieldValueType; // 值类型
    int32_t readOnly = 0;           // 是否只读 1是 0否
    std::string endianness;     // 字节序（Modbus使用）
    std::string description;    // 字段描述
};
// 设备
struct Device {
    int32_t id = 0;               // 设备ID
    int32_t pid = 0;              // 关联协议表ID
    std::string protocol;     // 协议类型
    std::string deviceName;   // 设备名称
    std::string description;  // 设备描述
    int32_t status = 0; ;        // 设备状态 1=正常 0=禁用
    bool connStatu = false;           //在线状态
    std::string ip;                 // 设备IP地址
    int32_t port = 0;               // 设备端口号（Modbus等协议使用）
    std::vector<CollectionField> CollectionFields;
};
class DeviceCollect : public DeviceInit{
public:
    DeviceCollect();
    ~DeviceCollect();
    //加载设备基础数据
    bool LoadDeviceData();
    // 添加协议类型
    bool AddProtocolType(const ProtocolType& protocolType);
    // 删除协议类型
    bool DeleteProtocolType(int id);
    // 添加设备
    bool AddDevice(const Device& device);
    // 删除设备
    bool DeleteDevice(int id);
    // 根据协议ID删除设备
    bool DeleteDevicesByProtocolId(int pid);
    // 添加采集字段
    bool AddCollectionField(const CollectionField& field);
    // 删除采集字段
    bool DeleteCollectionField(int id);
    // 根据设备ID删除采集字段
    bool DeleteCollectionFieldsByDeviceId(int did);
    //打印当前所有设备 协议
    void PrintAllDevices();
    //打印modbus协议
    void PrintModbusDevices();
private:
    //协议类型列表
    std::vector<ProtocolType> ProtocolTypes;
    //设备列表
    std::vector< Device> Devices;
    //根据ID获取设备信息
    Device* GetDeviceInfo(int id);
    //加载设备到协议池
    bool LoadDeviceToPool();
    //ModbusTcp 设备管理
    ModbusTcp::ModbusTcp ModbusTcpDevices;
    //是否初始化ModbusTcp
    bool isInitModbusTcp = false;
    //设备委托 将设备对应到相应的驱动中进行管理
    bool DevicesMapping();
    //modbusTcp 设备管理
    bool ModbusTcpDevicesMapping(Device& device);
    //初始化采集配置
    void initCollectConfig();
};

} // namespace DeviceMangement