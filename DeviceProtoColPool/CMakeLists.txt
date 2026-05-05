# CMakeList.txt: DeviceProtoColPool 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.8)

# 将源代码添加到此项目的可执行文件。
add_executable (DeviceProtoColPool "DeviceProtoColPool.cpp" "DeviceProtoColPool.h" "protocolPool/DB/sqlite.h" "protocolPool/DB/sqlite.cpp")

if(WIN32) #win环境
	find_package(httplib CONFIG REQUIRED)
	target_link_libraries(DeviceProtoColPool PRIVATE httplib::httplib)
	find_package(cJSON CONFIG REQUIRED)
	target_link_libraries(DeviceProtoColPool PRIVATE cjson)
	find_package(unofficial-sqlite3 CONFIG REQUIRED)
    target_link_libraries(DeviceProtoColPool PRIVATE unofficial::sqlite3::sqlite3)
endif()
if(UNIX) #linux环境
find_package(httplib CONFIG REQUIRED)
	target_link_libraries(DeviceProtoColPool PRIVATE httplib::httplib any)
	find_package(cJSON CONFIG REQUIRED)
	target_link_libraries(DeviceProtoColPool PRIVATE cjson)
endif()
# TODO: 如有需要，请添加测试并安装目标。
