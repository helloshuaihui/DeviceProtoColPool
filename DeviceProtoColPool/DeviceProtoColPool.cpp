#include "DeviceProtoColPool.h"
#include <iostream>
#include "protocolPool/DB/sqlite.h"
#pragma execution_character_set("utf-8")
#include <httplib.h>
#include "protocolPool/DeivceManagement/DeviceMangement.h"
#include "protocolPool/TOOL/ThreadPool.h"
extern  "C" {
}

using namespace std;
using namespace DB;
using namespace DeviceMangement;
//void testSQLite() {
//#ifdef _WIN32
//    string dbPath = "test.db";
//#else
//    string dbPath = "./test.db";
//#endif
//
//    SQLite db(dbPath);
//
//    if (db.open()) {
//        cout << "数据库连接成功" << endl;
//
//        if (!db.tableExists("devices")) {
//            bool created = db.createTable("devices", 
//                "id INTEGER PRIMARY KEY AUTOINCREMENT, "
//                "name TEXT NOT NULL, "
//                "type TEXT, "
//                "status INTEGER DEFAULT 0, "
//                "created_at TEXT");
//            if (created) {
//                cout << "表创建成功" << endl;
//            } else {
//                cout << "表创建失败: " << db.getLastErrorMessage() << endl;
//                return;
//            }
//        } else {
//            cout << "表已存在" << endl;
//        }
//
//        map<string, string> deviceData;
//        deviceData["name"] = "Device001";
//        deviceData["type"] = "Sensor";
//        deviceData["status"] = "1";
//        deviceData["created_at"] = "2024-01-01 12:00:00";
//
//        bool inserted = db.insert("devices", deviceData);
//        if (inserted) {
//            cout << "数据插入成功" << endl;
//        } else {
//            cout << "数据插入失败: " << db.getLastErrorMessage() << endl;
//        }
//
//        map<string, string> updateData;
//        updateData["status"] = "0";
//        bool updated = db.update("devices", updateData, "name = 'Device001'");
//        if (updated) {
//            cout << "数据更新成功" << endl;
//        } else {
//            cout << "数据更新失败: " << db.getLastErrorMessage() << endl;
//        }
//
//        auto result = db.query("SELECT * FROM devices");
//        cout << "查询结果 (" << result.size() << " 条):" << endl;
//        for (const auto& row : result) {
//            for (const auto& pair : row) {
//                cout << pair.first << ": " << pair.second << " ";
//            }
//            cout << endl;
//        }
//
//        /*bool removed = db.remove("devices", "name = 'Device001'");
//        if (removed) {
//            cout << "数据删除成功" << endl;
//        } else {
//            cout << "数据删除失败: " << db.getLastErrorMessage() << endl;
//        }*/
//
//        db.close();
//        cout << "数据库连接已关闭" << endl;
//
//        if (db.reconnect()) {
//            cout << "重新连接成功" << endl;
//            db.close();
//        } else {
//            cout << "重新连接失败: " << db.getLastErrorMessage() << endl;
//        }
//    } else {
//        cout << "数据库连接失败: " << db.getLastErrorMessage() << endl;
//    }
//}

int main()
{    
    #ifdef WIN32
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
    #endif // WIN32

    //DeviceCollect test;
    //
    //if (test.isConfigLoaded()) {
    //    cout << "配置加载成功" << endl;
    //    cout << "数据库路径: " << test.getDBPath() << endl;
    //    cout << "采集间隔: " << test.getCollectionInterval() << endl;
    //    cout << "自动加载数据库: " << (test.getIfAutoLoadDB() ? "是" : "否") << endl;
    //    cout << "自动重连: " << (test.getIsReconnect() ? "是" : "否") << endl;
    //    cout << "定时采集: " << (test.getIsTimedCollection() ? "是" : "否") << endl;
    //} else {
    //    cout << "配置加载失败" << endl;
    //    return -1;
    //}
    //cout << "开始加载数据库" << endl;
    //if (test.isDatabaseInitialized()) {
    //    cout << "数据库加载成功" << endl;
    //    cout << "数据库已就绪: " << test.getDBPath() << endl;
    //} else {
    //    cout << "数据库加载失败" << endl;
    //    return -1;
    //}
    //test.PrintAllDevices();
    
    //线程调用
    //int i = 0;
    //TOOL::ThreadPool tp(6,1000);
    //for (int j = 0; j < 1000;j++) {
    //    tp.submit([&i]() -> void {
    //        i += 1;
    //        cout << "i=" << i << endl;
    //        });
    //}

    //Sleep(1000000);
    return 0;
}