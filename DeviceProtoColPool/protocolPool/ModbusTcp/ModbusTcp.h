#pragma once
#ifndef MODBUSTCP_H 
#define MODBUSTCP_H
#include "../TCP/TcpSocketClass.h"
namespace TCP {
	//数据类型
	enum class BuffType :int {
		D_UINT8 = 1,
		D_UINT16 = 2,
		D_UINT32 = 3,
		D_INT16 = 4,
		D_INT32 = 5,
		D_FLOAT = 6
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
	class ModbusTcp : TcpSocketClass
	{
	public:
		ModbusTcp();
		~ModbusTcp();
		//读寄存器
		bool readRegister(ModbusTcpInfo &modbusTcpInfo, RegisterBuf &registerBuf);
		//写寄存器
		bool writeRegister(ModbusTcpInfo &modbusTcpInfo, RegisterBuf &registerBuf);
		//读线圈
		bool readCoil(ModbusTcpInfo &modbusTcpInfo, RegisterBuf &registerBuf);
		//写线圈
		bool writeCoil(ModbusTcpInfo &modbusTcpInfo, RegisterBuf &registerBuf);
		//寄存缓冲区解析
		bool parseRegisterBuf(char* buff, RegisterBuf &registerBuf);
		//线圈缓冲区解析
		bool parseCoilBuf(ModbusTcpInfo &modbusTcpInfo, RegisterBuf &registerBuf);
	private:
	};

}
#endif // !MODBUSTCP_H
