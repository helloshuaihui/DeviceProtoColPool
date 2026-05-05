// DeviceProtoColPool.cpp: 定义应用程序的入口点。
//

#include "DeviceProtoColPool.h"
#include <httplib.h>
#include <cjson/cJSON.h>
#include <iostream>
#include "protocolPool/DB/sqlite.h"

using namespace std;
using namespace DB;

void testSQLite() {
#ifdef _WIN32
    string dbPath = "test.db";
#else
    string dbPath = "./test.db";
#endif

    SQLite db(dbPath);

    if (db.open()) {
        cout << "数据库连接成功" << endl;

        if (!db.tableExists("devices")) {
            bool created = db.createTable("devices", 
                "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                "name TEXT NOT NULL, "
                "type TEXT, "
                "status INTEGER DEFAULT 0, "
                "created_at TEXT");
            if (created) {
                cout << "表创建成功" << endl;
            } else {
                cout << "表创建失败: " << db.getLastErrorMessage() << endl;
                return;
            }
        } else {
            cout << "表已存在" << endl;
        }

        map<string, string> deviceData;
        deviceData["name"] = "Device001";
        deviceData["type"] = "Sensor";
        deviceData["status"] = "1";
        deviceData["created_at"] = "2024-01-01 12:00:00";

        bool inserted = db.insert("devices", deviceData);
        if (inserted) {
            cout << "数据插入成功" << endl;
        } else {
            cout << "数据插入失败: " << db.getLastErrorMessage() << endl;
        }

        map<string, string> updateData;
        updateData["status"] = "0";
        bool updated = db.update("devices", updateData, "name = 'Device001'");
        if (updated) {
            cout << "数据更新成功" << endl;
        } else {
            cout << "数据更新失败: " << db.getLastErrorMessage() << endl;
        }

        auto result = db.query("SELECT * FROM devices");
        cout << "查询结果 (" << result.size() << " 条):" << endl;
        for (const auto& row : result) {
            for (const auto& pair : row) {
                cout << pair.first << ": " << pair.second << " ";
            }
            cout << endl;
        }

        /*bool removed = db.remove("devices", "name = 'Device001'");
        if (removed) {
            cout << "数据删除成功" << endl;
        } else {
            cout << "数据删除失败: " << db.getLastErrorMessage() << endl;
        }*/

        db.close();
        cout << "数据库连接已关闭" << endl;

        if (db.reconnect()) {
            cout << "重新连接成功" << endl;
            db.close();
        } else {
            cout << "重新连接失败: " << db.getLastErrorMessage() << endl;
        }
    } else {
        cout << "数据库连接失败: " << db.getLastErrorMessage() << endl;
    }
}

int main()
{    
    testSQLite();
    
    return 0;
}