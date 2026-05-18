#pragma once
#ifndef MODBUSTCP_H 
#define MODBUSTCP_H
#include <unordered_map>
#include "../TCP/TcpSocketClass.h"
#include "../TOOL/ThreadPool.h"
namespace ModbusTcp {
	//数据类型
	enum class BuffType :int {
		D_UINT8 = 1,
		D_UINT16 = 2,
		D_UINT32 = 3,
		D_INT16 = 4,
		D_INT32 = 5,
		D_FLOAT = 6,
		D_UINT64 = 7,
		D_INT64 = 8,
	};
	//字节序
	enum class ByteSequence :int
	{
		ABCD = 1,
		BADC = 2,
		CDAB = 3,
		DCBA = 4
	};
	//modbus连接基础信息
	struct ModbusTcpInfo
	{
		uint16_t slaveId=0;
		uint16_t port=0;
		std::string ip="0.0.0.0";
		TCPSOCK sock=0;
		bool conn=false;
		std::string id="";
		std::string errMsg = "";
		ModbusTcpInfo(std::string _ip,uint16_t _port) {
			ip = _ip;
			port = _port;
		}
	};
	//寄存器数据
	struct RegisterBuf
	{
		uint16_t address=0;
		uint16_t registerLen = 1;
		std::string RegisterName="";
		BuffType buffType=BuffType::D_UINT16;
		ByteSequence byteSequence = ByteSequence::ABCD;
		uint8_t uint8Buf = 0;
		uint16_t uint16Buf = 0;
		uint32_t uint32Buf = 0;
		int16_t int16Buf = 0;
		int32_t int32Buf = 0;
		float floatBuf = 0;
		RegisterBuf(uint16_t _address,BuffType _buffType = BuffType::D_UINT16, ByteSequence _byteSequence= ByteSequence::ABCD) {
			address = _address;
			buffType = _buffType;
			switch (buffType)
			{
			case BuffType::D_UINT8:
				registerLen = 1;
				break;
			case BuffType::D_UINT16:
				registerLen = 1;
				break;
			case BuffType::D_UINT32:
				registerLen = 2;
				break;
			case BuffType::D_INT16:
				registerLen = 1;
				break;
			case BuffType::D_INT32:
				registerLen = 2;
				break;
			case BuffType::D_FLOAT:
				registerLen = 2;
				break;
			}
		}
		template<typename T>
		T getData() {
			switch (buffType)
			{
			case TCP::D_UINT8:
				return uint8Buf;
			case TCP::D_UINT16:
				return uint16Buf;
			case TCP::D_UINT32:
				return uint32Buf;
			case TCP::D_INT16:
				return int16Buf;
			case TCP::D_INT32:
				return int32Buf;
			case TCP::D_FLOAT:
				return floatBuf;
			default:
				break;
			}
		}
	};
	// 采集字段定义
	struct CollectionField {
		int32_t id = 0;                 // 字段ID
		int32_t did = 0;                // 关联设备ID
		int fieldAddress = 0;	    	// 寄存器地址
		int32_t registerLen = 0;      // 寄存器长度
		BuffType buffType = BuffType::D_UINT16;
		int32_t readOnly = 0;        // 是否只读 1是 0否
		ByteSequence byteSequence = ByteSequence::ABCD;
		uint8_t uint8Data = 0;
		uint16_t uint16Data = 0;
		uint32_t uint32Data = 0;
		int16_t int16Data = 0;
		int32_t int32Data = 0;
		uint64_t uint64Data = 0;
		int64_t int64Data = 0;
		float floatData = 0;
		//获取数据
		template<typename T>
		T getData() {
			switch (buffType)
			{
			case BuffType::D_UINT8:
				return uint8Data;
			case BuffType::D_UINT16:
				return uint16Data;
			case BuffType::D_UINT32:
				return uint32Data;
			case BuffType::D_INT16:
				return int16Data;
			case BuffType::D_INT32:
				return int32Data;
			case BuffType::D_FLOAT:
				return floatData;
			default:
				break;
			}
		}
		//设置数据
		template<typename T>
		void setData(T data) {
			switch (buffType)
			{
			case BuffType::D_UINT8:
				uint8Data = data;
				break;
			case BuffType::D_UINT16:
				uint16Data = data;
				break;
			case BuffType::D_UINT32:
				uint32Data = data;
				break;
			case BuffType::D_INT16:
				int16Data = data;
				break;
			case BuffType::D_INT32:
				int32Data = data;
				break;
			case BuffType::D_FLOAT:
				floatData = data;
				break;
			default:
				break;
			}
		}
		//设置寄存器长度
		void setRegisterLen() {
			switch (buffType)
			{
			case BuffType::D_UINT8:
				registerLen = 1;
				break;
			case BuffType::D_UINT16:
				registerLen = 2;
				break;
			case BuffType::D_UINT32:
				registerLen = 4;
				break;
			case BuffType::D_INT16:
				registerLen = 2;
				break;
			case BuffType::D_INT32:
				registerLen = 4;
				break;
			case BuffType::D_FLOAT:
				registerLen = 4;
				break;
			case BuffType::D_INT64:
				registerLen = 8;
				break;
			case BuffType::D_UINT64:
				registerLen = 8;
				break;
			default:
				break;
			}
		}
		/*
			设置字节序以及值类型
			byteOrder=ABCD/CDAB/BADC/DCBA
			valueType=uint8/uint32/uint64/int16/int32/int64/float
		*/
		void SetValueInfo(std::string byteOrder="ABCD", std::string valueType = "uint16") {
			if (valueType.compare("uint8") == 0) {
				buffType = BuffType::D_UINT8;
				registerLen = 1;
			}
			else if (valueType.compare("uint16") == 0) {
				buffType = BuffType::D_UINT16;
				registerLen = 2;
			}
			else if (valueType.compare("uint32") == 0) {
				buffType = BuffType::D_UINT32;
				registerLen = 4;
			}
			else if (valueType.compare("uint64") == 0) {
				buffType = BuffType::D_UINT64;
				registerLen = 8;
			}
			else if (valueType.compare("int16") == 0) {
				buffType = BuffType::D_INT16;
				registerLen = 2;
			}
			else if (valueType.compare("int32") == 0) {
				buffType = BuffType::D_INT32;
				registerLen = 4;
			}
			else if (valueType.compare("int64") == 0) {
				registerLen = 8;
				buffType = BuffType::D_INT64;
			}
			else if (valueType.compare("float") == 0) {
				buffType = BuffType::D_FLOAT;
				registerLen = 4;
			}
			if (byteOrder.compare("ABCD") == 0) {
				byteSequence = ByteSequence::ABCD;
			}
			else if (byteOrder.compare("BADC") == 0) {
				byteSequence = ByteSequence::BADC;
			}
			else if (byteOrder.compare("CDAB") == 0) {
				byteSequence = ByteSequence::CDAB;
			}
			else if (byteOrder.compare("DCBA") == 0) {
				byteSequence = ByteSequence::DCBA;
			}
		}
	};
	// 设备定义
	struct ModbusTcpDevice {
		int32_t id = 0;               	// 设备ID
		int32_t status = 0; ;        	// 设备状态 1=正常 0=禁用
		bool connStatu = false;         //在线状态
		TCPSOCK sock = 0;				// 设备socket
		std::string ip;                 // 设备IP地址
		int32_t port = 0;               // 设备端口号
		uint16_t slaveId = 1;           // 从站地址/从站ID，默认1
		std::string errMsg;				//错误信息
		std::vector<CollectionField> CollectionFields;
	};
	class ModbusTcp : TCP::TcpSocketClass
	{
	public:
		ModbusTcp();
		~ModbusTcp();
		//是否定时采集数据
		bool isTimedCollection = false;
		//定时采集时间间隔
		int32_t CollectionInterval = 0;
		//最大连接时间
		int32_t MaxConnectionTime = 0;
		//是否重连
		bool isReconnect = false;
		//重连时间间隔
		int32_t ReconnectInterval = 0;
		//添加设备
		bool addDevice(ModbusTcpDevice& modbusTcpDevice);
		//删除设备
		bool delDevice(int id);
		//获取所有设备
		std::vector<ModbusTcpDevice> GetAllDevices();
		//初始化设备连接
		void initDeviceConn();
		//定时任务
		void timerTask();
	private:
		//modbus设备列表
		std::vector<ModbusTcpDevice> modbusTcpDevices;
		//线程池
		TOOL::ThreadPool* tPool;
		//线程池是否初始化
		bool isInitThreadPool = false;
		//设备锁操作
		std::mutex deviceMutex;
		//数据锁操作
		std::mutex dataMutex;
		//事务ID计数器
		std::atomic<uint16_t> transactionId{1};
		//获取下一个事务ID（线程安全）
		uint16_t getNextTransactionId();
		//事务ID到字段的映射（用于多线程响应匹配）
		std::mutex transactionMutex;
		std::unordered_map<uint16_t, std::pair<ModbusTcpDevice*, CollectionField*>> transactionMap;
		//设备连接检查以及重连
		void checkDeviceConn();
		//获取所有设备数据
		std::vector<ModbusTcpDevice> getAllDeviceData();
		//更新设备数据
		void updateDeviceData();
		//获取当个设备数据
		ModbusTcpDevice getDeviceData(int did);
		//获取单个设备单个字段数据
		template<typename T>
		T getDeviceFieldData(int did, int field);
		//读寄存器
		bool readRegister(ModbusTcpDevice& modbusTcpDevice, CollectionField& collectionField);
		//写寄存器
		bool writeRegister(ModbusTcpInfo& modbusTcpInfo, RegisterBuf& registerBuf);
		//读线圈
		bool readCoil(ModbusTcpInfo& modbusTcpInfo, RegisterBuf& registerBuf);
		//写线圈
		bool writeCoil(ModbusTcpInfo& modbusTcpInfo, RegisterBuf& registerBuf);
		//寄存缓冲区解析
		bool parseRegisterBuf(char* buff, CollectionField& collectionField);
		//线圈缓冲区解析
		bool parseCoilBuf(ModbusTcpInfo& modbusTcpInfo, RegisterBuf& registerBuf);
	};

	template<typename T>
	inline T ModbusTcp::getDeviceFieldData(int did, int field)
	{
		return T();
	}

}
#endif // !MODBUSTCP_H
