#include "ModbusTcp.h"
namespace TCP {
	ModbusTcp::ModbusTcp()
	{
	}

	ModbusTcp::~ModbusTcp()
	{
	}
    //读寄存器
    bool ModbusTcp::readRegister(ModbusTcpInfo &modbusTcpInfo, RegisterBuf &registerBuf)
    {
        if (modbusTcpInfo.sock <= 0) {
            TCPSOCK sock=this->connTcpScokerServer(modbusTcpInfo.ip, modbusTcpInfo.port);
            if (!sock > 0) { modbusTcpInfo.errMsg = "Failed to connect to Modbus TCP"; return false; }
            modbusTcpInfo.sock = sock;

        }
        //构建请求 
        char headBuf[13] = { 0 };
        headBuf[0] = 0x00;
        headBuf[1] = 0x01;
        headBuf[2] = 0x00;
        headBuf[3] = 0x00;
        headBuf[4] = 0x00;
        headBuf[5] = 0x06;
        headBuf[6] = 0x01;
        headBuf[7] = 0x03;
        uint16_t addr = registerBuf.address;
        headBuf[8] = (addr >> 8) & 0xFF;
        headBuf[9] = addr & 0xFF;
        uint16_t rlen = registerBuf.registerLen;
        headBuf[10] = (rlen >> 8) & 0xFF;
        headBuf[11] = rlen & 0xFF;
		int slen=send(modbusTcpInfo.sock, headBuf, 12, 0);
		if(slen<=0){
            modbusTcpInfo.errMsg = "Message sending failed";
			return false;
		}else{
			//接收响应
			char recvBuf[100] = { 0 };
			int recvLen=recv(modbusTcpInfo.sock, recvBuf, 100, 0);
			if(recvLen<=0){
                modbusTcpInfo.sock = 0;
                modbusTcpInfo.errMsg = "Message reception failed, connection has been disconnected";
				return false;
			}
			//解析响应
			parseRegisterBuf(recvBuf, registerBuf);
            modbusTcpInfo.errMsg = "Data received successfully";
			return true;
		}
        return false;
    }
    //写寄存器
    bool ModbusTcp::writeRegister(ModbusTcpInfo& modbusTcpInfo, RegisterBuf& registerBuf)
    {
        if (modbusTcpInfo.sock <= 0) {
            TCPSOCK sock = this->connTcpScokerServer(modbusTcpInfo.ip, modbusTcpInfo.port);
            if (sock <= 0) { return false; }
            modbusTcpInfo.sock = sock;
        }

        // 发送缓冲区
        uint8_t sendBuf[50] = { 0 };
        int sendLen = 0;
        static uint16_t transID = 1;
        sendBuf[0] = 0x00;
        sendBuf[1] = 0x01;
        sendBuf[2] = 0x00;
        sendBuf[3] = 0x00;
        sendBuf[6] = 0x01;
        if (registerBuf.buffType == BuffType::D_UINT16 ||
            registerBuf.buffType == BuffType::D_INT16 ||
            registerBuf.buffType == BuffType::D_UINT8)
        {
            sendBuf[7] = 0x06;                
            sendBuf[4] = 0x00;
            sendBuf[5] = 0x06;    

            // 地址
            uint16_t addr = registerBuf.address;
            sendBuf[8] = (addr >> 8) & 0xFF;
            sendBuf[9] = addr & 0xFF;
            // 取值
            uint16_t val = 0;
            if (registerBuf.buffType == BuffType::D_UINT16) val = registerBuf.uint16Buf;
            else if (registerBuf.buffType == BuffType::D_INT16) val = (uint16_t)registerBuf.int16Buf;
            else if (registerBuf.buffType == BuffType::D_UINT8) val = registerBuf.uint8Buf;
            // 字节序
            if (registerBuf.byteSequence == ByteSequence::BADC) {
                sendBuf[10] = val & 0xFF;
                sendBuf[11] = (val >> 8) & 0xFF;
            }
            else {
                sendBuf[10] = (val >> 8) & 0xFF;
                sendBuf[11] = val & 0xFF;
            }

            sendLen = 12;
        }
        else if (registerBuf.buffType == BuffType::D_UINT32 ||
            registerBuf.buffType == BuffType::D_INT32 ||
            registerBuf.buffType == BuffType::D_FLOAT)
        {
            sendBuf[7] = 0x10;                // 功能码：写多个
            sendBuf[4] = 0x00;
            sendBuf[5] = 0x0B;                // 长度 = 11
            // 地址
            uint16_t addr = registerBuf.address;
            sendBuf[8] = (addr >> 8) & 0xFF;
            sendBuf[9] = addr & 0xFF;
            sendBuf[10] = 0x00;
            sendBuf[11] = 0x02;
            sendBuf[12] = 0x04;
            uint32_t val32 = 0;
            if (registerBuf.buffType == BuffType::D_UINT32)      val32 = registerBuf.uint32Buf;
            else if (registerBuf.buffType == BuffType::D_INT32)  val32 = (uint32_t)registerBuf.int32Buf;
            else if (registerBuf.buffType == BuffType::D_FLOAT)  memcpy(&val32, &registerBuf.floatBuf, 4);
            uint8_t b0 = (val32 >> 24) & 0xFF;
            uint8_t b1 = (val32 >> 16) & 0xFF;
            uint8_t b2 = (val32 >> 8) & 0xFF;
            uint8_t b3 = val32 & 0xFF;

            switch (registerBuf.byteSequence)
            {
            case ByteSequence::ABCD: sendBuf[13] = b0; sendBuf[14] = b1; sendBuf[15] = b2; sendBuf[16] = b3; break;
            case ByteSequence::BADC: sendBuf[13] = b1; sendBuf[14] = b0; sendBuf[15] = b3; sendBuf[16] = b2; break;
            case ByteSequence::CDAB: sendBuf[13] = b2; sendBuf[14] = b3; sendBuf[15] = b0; sendBuf[16] = b1; break;
            case ByteSequence::DCBA: sendBuf[13] = b3; sendBuf[14] = b2; sendBuf[15] = b1; sendBuf[16] = b0; break;
            default: sendBuf[13] = b0; sendBuf[14] = b1; sendBuf[15] = b2; sendBuf[16] = b3; break;
            }
            sendLen = 17;
        }
        int slen = send(modbusTcpInfo.sock, (char*)sendBuf, sendLen, 0);
        if (slen <= 0) {
            modbusTcpInfo.sock = 0;
            return false;
        }
        uint8_t recvBuf[50] = { 0 };
        int recvLen = recv(modbusTcpInfo.sock, (char*)recvBuf, 50, 0);

        if (recvLen <= 0) {
            modbusTcpInfo.sock = 0;
            return false;
        }
        if (recvBuf[7] == sendBuf[7]) {
            return true;
        }
        return false;
    }
    //读线圈
    bool ModbusTcp::readCoil(ModbusTcpInfo& modbusTcpInfo, RegisterBuf& registerBuf)
    {
        if (modbusTcpInfo.sock <= 0) {
            TCPSOCK sock=this->connTcpScokerServer(modbusTcpInfo.ip, modbusTcpInfo.port);
            if (sock <= 0) { return false; }
            modbusTcpInfo.sock = sock;

        }
        //构建请求 
        char headBuf[12] = { 0 };
        headBuf[0] = 0x00;
        headBuf[1] = 0x01;
        headBuf[2] = 0x00;
        headBuf[3] = 0x00;
        headBuf[4] = 0x00;
        headBuf[5] = 0x06;
        headBuf[6] = 0x01;
        headBuf[7] = 0x01; // 读线圈功能码
        uint16_t addr = registerBuf.address;
        headBuf[8] = (addr >> 8) & 0xFF;
        headBuf[9] = addr & 0xFF;
        uint16_t rlen = registerBuf.registerLen;
        headBuf[10] = (rlen >> 8) & 0xFF;
        headBuf[11] = rlen & 0xFF;
        
        int slen=send(modbusTcpInfo.sock, headBuf, 12, 0);
        if(slen<=0){
            modbusTcpInfo.sock = 0;
            return false;
        }else{
            //接收响应
            char recvBuf[100] = { 0 };
            int recvLen=recv(modbusTcpInfo.sock, recvBuf, 100, 0);
            if(recvLen<=0){
                modbusTcpInfo.sock = 0;
                return false;
            }
            //解析响应
            // 响应格式：事务ID(2) + 协议ID(2) + 长度(2) + 从站地址(1) + 功能码(1) + 字节数(1) + 数据(n)
            if (recvLen < 8) {
                return false;
            }
            
            // 验证从站地址和功能码
            if (recvBuf[6] != headBuf[6] || recvBuf[7] != headBuf[7]) {
                return false;
            }
            
            // 解析线圈状态
            int byteCount = recvBuf[8];
            if (byteCount > 0) {
                // 对于单个线圈，只取第一个字节的最低位
                if (registerBuf.registerLen == 1) {
                    registerBuf.uint8Buf = (recvBuf[9] & 0x01) ? 1 : 0;
                } else {
                    // 对于多个线圈，可以根据需要扩展解析逻辑
                    registerBuf.uint8Buf = (recvBuf[9] & 0x01) ? 1 : 0;
                }
            }
            return true;
        }
        return false;
    }
    //写线圈
    bool ModbusTcp::writeCoil(ModbusTcpInfo& modbusTcpInfo, RegisterBuf& registerBuf)
    {
        if (modbusTcpInfo.sock <= 0) {
            TCPSOCK sock=this->connTcpScokerServer(modbusTcpInfo.ip, modbusTcpInfo.port);
            if (sock <= 0) { return false; }
            modbusTcpInfo.sock = sock;

        }
        
        if (registerBuf.registerLen == 1) {
            // 写单个线圈，使用功能码0x05
            char headBuf[12] = { 0 };
            headBuf[0] = 0x00;
            headBuf[1] = 0x01;
            headBuf[2] = 0x00;
            headBuf[3] = 0x00;
            headBuf[4] = 0x00;
            headBuf[5] = 0x06;
            headBuf[6] = 0x01;
            headBuf[7] = 0x05; // 写单个线圈功能码
            uint16_t addr = registerBuf.address;
            headBuf[8] = (addr >> 8) & 0xFF;
            headBuf[9] = addr & 0xFF;
            // 线圈状态：0xFF00为ON，0x0000为OFF
            uint16_t state = registerBuf.uint8Buf ? 0xFF00 : 0x0000;
            headBuf[10] = (state >> 8) & 0xFF;
            headBuf[11] = state & 0xFF;
            
            int slen=send(modbusTcpInfo.sock, headBuf, 12, 0);
            if(slen<=0){
                modbusTcpInfo.sock = 0;
                return false;
            }else{
                //接收响应
                char recvBuf[100] = { 0 };
                int recvLen=recv(modbusTcpInfo.sock, recvBuf, 100, 0);
                if(recvLen<=0){
                    modbusTcpInfo.sock = 0;
                    return false;
                }
                // 验证响应
                if (recvLen != 12) {
                    return false;
                }
                
                // 验证从站地址和功能码
                if (recvBuf[6] != headBuf[6] || recvBuf[7] != headBuf[7]) {
                    return false;
                }
                
                // 验证线圈地址和状态
                if (recvBuf[8] != headBuf[8] || recvBuf[9] != headBuf[9] ||
                    recvBuf[10] != headBuf[10] || recvBuf[11] != headBuf[11]) {
                    return false;
                }
                return true;
            }
        } else {
            // 写多个线圈，使用功能码0x0F
            int byteCount = (registerBuf.registerLen + 7) / 8;
            char headBuf[256] = { 0 };
            headBuf[0] = 0x00;
            headBuf[1] = 0x01;
            headBuf[2] = 0x00;
            headBuf[3] = 0x00;
            headBuf[4] = 0x00;
            headBuf[5] = 0x07 + byteCount; // 长度
            headBuf[6] = 0x01;
            headBuf[7] = 0x0F; // 写多个线圈功能码
            uint16_t addr = registerBuf.address;
            headBuf[8] = (addr >> 8) & 0xFF;
            headBuf[9] = addr & 0xFF;
            uint16_t count = registerBuf.registerLen;
            headBuf[10] = (count >> 8) & 0xFF;
            headBuf[11] = count & 0xFF;
            headBuf[12] = byteCount; // 字节数
            
            // 这里简化处理，只设置第一个线圈的状态
            // 实际应用中需要根据具体需求设置多个线圈的状态
            headBuf[13] = registerBuf.uint8Buf ? 0x01 : 0x00;
            
            int sendLen = 13 + byteCount;
            int slen=send(modbusTcpInfo.sock, headBuf, sendLen, 0);
            if(slen<=0){
                modbusTcpInfo.sock = 0;
                return false;
            }else{
                //接收响应
                char recvBuf[100] = { 0 };
                int recvLen=recv(modbusTcpInfo.sock, recvBuf, 100, 0);
                if(recvLen<=0){
                    modbusTcpInfo.sock = 0;
                    return false;
                }
                // 验证响应
                if (recvLen != 8) {
                    return false;
                }
                
                // 验证从站地址和功能码
                if (recvBuf[6] != headBuf[6] || recvBuf[7] != headBuf[7]) {
                    return false;
                }
                
                // 验证起始地址和线圈数量
                if (recvBuf[8] != headBuf[8] || recvBuf[9] != headBuf[9] ||
                    recvBuf[10] != headBuf[10] || recvBuf[11] != headBuf[11]) {
                    return false;
                }
                return true;
            }
        }
        return false;
    }
	//解析读寄存器
    bool ModbusTcp::parseRegisterBuf(char* buff, RegisterBuf& registerBuf)
	{
		int dataBitLen = 1;
        if(registerBuf.buffType == BuffType::D_UINT8) {
            dataBitLen = 1;
            int len = buff[8] / dataBitLen;
            for (int i = 0; i < len; i++) {
                char bufData = buff[9 + i];
                registerBuf.uint8Buf = bufData;
            }
        }
        else if (registerBuf.buffType == BuffType::D_UINT16 || registerBuf.buffType == BuffType::D_INT16) {
            dataBitLen = 2;

            int len = buff[8] / dataBitLen;
            for (int i = 0; i < len * 2; i += 2) {
                uint16_t bufData;
                switch (registerBuf.byteSequence)
                {
                case ByteSequence::ABCD:
                    bufData = ((buff[9 + i] << 8) & 0xffff) | (buff[9 + i + 1] & 0x00ff);
                    break;
                case ByteSequence::BADC:
                    bufData = ((buff[9 + i + 1] << 8) & 0xffff) | (buff[9 + i] & 0x00ff);
                    break;
                default:
                    bufData = ((buff[9 + i] << 8) & 0xffff) | (buff[9 + i + 1] & 0x00ff);
                    break;
                }
                if (registerBuf.buffType == BuffType::D_UINT16) {
                    registerBuf.uint16Buf = bufData;
                }
                else {
                    int16_t int16Data = *reinterpret_cast<int16_t*>(&bufData);
                    registerBuf.int16Buf = int16Data;
                }
            }
        }
        else {
            dataBitLen = 4;
            int len = buff[8] / dataBitLen;
            for (int i = 0; i < len * 4; i += 4) {
                uint32_t bufData = 0;
                switch (registerBuf.byteSequence)
                {
                case ByteSequence::ABCD:
                    bufData = ((buff[9 + i] & 0xFF) << 24) | ((buff[9 + i + 1] & 0xFF) << 16) |
                        ((buff[9 + i + 2] & 0xFF) << 8) | (buff[9 + i + 3] & 0xFF);
                    break;
                case ByteSequence::BADC:
                    bufData = ((buff[9 + i + 1] & 0xFF) << 24) | ((buff[9 + i] & 0xFF) << 16) |
                        ((buff[9 + i + 3] & 0xFF) << 8) | (buff[9 + i + 2] & 0xFF);
                    break;
                case ByteSequence::CDAB:
                    bufData = ((buff[9 + i + 2] & 0xFF) << 24) | ((buff[9 + i + 3] & 0xFF) << 16) |
                        ((buff[9 + i] & 0xFF) << 8) | (buff[9 + i + 1] & 0xFF);
                    break;
                default: //DCBA
                    bufData = ((buff[9 + i + 3] & 0xFF) << 24) | ((buff[9 + i + 2] & 0xFF) << 16) |
                        ((buff[9 + i + 1] & 0xFF) << 8) | (buff[9 + i] & 0xFF);
                    break;
                }
                switch (registerBuf.buffType)
                {
                case BuffType::D_UINT32:
                    registerBuf.uint32Buf = bufData;
                    break;
                case BuffType::D_FLOAT:
                    float floatData; //转换成小数存储
                    memcpy(&floatData, &bufData, sizeof(floatData));
                    registerBuf.floatBuf = floatData;
                    break;
                default: //BuffType::INT32
                    int32_t int32Data = static_cast<int32_t>(bufData);
                    registerBuf.int32Buf = int32Data;
                    break;
                }
            }
        }
		return true;
	}
    //解析读线圈
    bool ModbusTcp::parseCoilBuf(ModbusTcpInfo& modbusTcpInfo, RegisterBuf& registerBuf)
    {
        return false;
    }
}