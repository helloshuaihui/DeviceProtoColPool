#pragma once
#include <cjson/cJSON.h>
#include <string>
#include <iostream>
#include <fstream>
#include <string>
#ifdef _WIN32
    #include <windows.h>
    #include <direct.h> 
#else
    #include <unistd.h>
    #include <sys/stat.h>
#endif
#include "../DB/sqlite.h"
namespace DeviceMangement {

class DeviceInit {
public:
	//构造函数
    DeviceInit(const std::string& configPath = "DeviceCollectConfig.json");
    //析构函数
    ~DeviceInit();
	//加载配置文件
    bool loadConfig();
	//加载数据库（检查并创建数据表）
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
	//创建数据库表
    bool createTables(DB::SQLite& db);
	//判断文件是否存在
    bool fileExists(const std::string& path) const;
    //创建文件
    bool createFile(const std::string& filePath);
};

class DeviceCollect {
public:
    DeviceCollect();
    ~DeviceCollect();

private:

};

} // namespace DeviceMangement