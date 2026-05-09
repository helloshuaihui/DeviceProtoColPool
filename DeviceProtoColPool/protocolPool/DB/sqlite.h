#pragma once
#include "sqlite3.h"
#include <string>
#include <vector>
#include <map>
#include <mutex>

namespace DB {

class SQLite {
public:
    SQLite(const std::string& dbPath="");
    ~SQLite();
    //数据库路径
    std::string m_dbPath;
    //打开链接
    bool open();
    //关闭链接
    void close();
    //判断是否打开
    bool isOpen() const;
    //执行SQL语句
    bool execute(const std::string& sql);
    //插入数据
    bool insert(const std::string& tableName, const std::map<std::string, std::string>& data);
    //更新数据
    bool update(const std::string& tableName, const std::map<std::string, std::string>& data, const std::string& whereClause);
    //删除数据
    bool remove(const std::string& tableName, const std::string& whereClause);
    //查询数据 并返回结果
    std::vector<std::map<std::string, std::string>> query(const std::string& sql);
    //判断表是否存在
    bool tableExists(const std::string& tableName);
    //创建表
    bool createTable(const std::string& tableName, const std::string& columns);
    //重新连接数据库
    bool reconnect();
    //获取最后一个错误码
    int getLastError() const;
    //获取最后一个错误信息
    std::string getLastErrorMessage() const;
    //判断文件是否存在
    bool fileExists(const std::string& path) const;
private:
    //数据库句柄
    sqlite3* m_db;
    //互斥锁
    mutable std::mutex m_mutex;
    //最后一个错误码
    int m_lastError;
    //最后一个错误信息
    std::string m_lastErrorMessage;
    
    //设置最后一个错误码和信息
    void setLastError(int code, const std::string& message);
    //查询回调函数
    static int queryCallback(void* data, int argc, char** argv, char** azColName);
};

} // namespace DB
