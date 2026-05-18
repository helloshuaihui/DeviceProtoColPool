#include "ModbusTcp.h" 
namespace ModbusTcp {
	ModbusTcp::ModbusTcp()
	{
	}

	ModbusTcp::~ModbusTcp()
	{}
	
	// 获取下一个事务ID，线程安全，从1递增到65535后回绕到0再继续递增
	uint16_t ModbusTcp::getNextTransactionId() {
		uint16_t current = transactionId.fetch_add(1);
		if (current == 65535) {
			transactionId.store(0);
			return 65535;
		}
		return current;
	}
	
    bool ModbusTcp::addDevice(ModbusTcpDevice &modbusTcpDevice)
    {
        std::lock_guard<std::mutex> lock(deviceMutex);
        for(int i = 0; i < modbusTcpDevices.size(); i++){
            if(modbusTcpDevices[i].id == modbusTcpDevice.id){
                return false;
            }
        }
        modbusTcpDevices.push_back(modbusTcpDevice);
        return true;
    }
    bool ModbusTcp::delDevice(int id)
    {
        std::lock_guard<std::mutex> lock(deviceMutex);
        for(int i = 0; i < modbusTcpDevices.size(); i++){
            if(modbusTcpDevices[i].id == id){
                modbusTcpDevices.erase(modbusTcpDevices.begin() + i);
                return true;
            }
        }
        return false;
    }
    std::vector<ModbusTcpDevice> ModbusTcp::GetAllDevices()
    {
        std::lock_guard<std::mutex> lock(deviceMutex);
        return this->modbusTcpDevices;
    }
    void ModbusTcp::initDeviceConn()
    {
        if(!this->isInitThreadPool){
            this->tPool = new TOOL::ThreadPool(4,1000);
            this->isInitThreadPool = true;
        }
        std::lock_guard<std::mutex> lock(deviceMutex);
        for (size_t i = 0; i < this->modbusTcpDevices.size(); ++i) {
            size_t index = i;
            this->tPool->submit([this, index]()->void {
                TCPSOCK sock = this->connTcpScokerServer(this->modbusTcpDevices[index].ip, this->modbusTcpDevices[index].port, this->MaxConnectionTime);
                if (sock <= 0) { 
                    std::lock_guard<std::mutex> lock(this->deviceMutex);
                    this->modbusTcpDevices[index].errMsg = "Failed to connect to Modbus TCP"; 
                    return; 
                }
                std::lock_guard<std::mutex> lock(this->deviceMutex);
                this->modbusTcpDevices[index].sock = sock; 
                this->modbusTcpDevices[index].connStatu = true;
            });
        }
    }
    void ModbusTcp::timerTask()
    {
        tPool->submit([this]() -> void {
            while (true)
            {
                Sleep(1000);
                updateDeviceData();
            }
        });
    }
    void ModbusTcp::checkDeviceConn()
    {
        std::lock_guard<std::mutex> lock(deviceMutex);
        for (size_t i = 0; i < this->modbusTcpDevices.size(); ++i) {
            if(this->modbusTcpDevices[i].connStatu){
                continue;
            }else{
                if(this->isReconnect){
                    size_t index = i;
                    this->tPool->submit([this, index]()->void {
                        TCPSOCK sock = this->connTcpScokerServer(this->modbusTcpDevices[index].ip, this->modbusTcpDevices[index].port, this->MaxConnectionTime);
                        if (sock <= 0) { 
                            std::lock_guard<std::mutex> lock(this->deviceMutex);
                            this->modbusTcpDevices[index].errMsg = "Failed to connect to Modbus TCP"; 
                            return; 
                        }
                        std::lock_guard<std::mutex> lock(this->deviceMutex);
                        this->modbusTcpDevices[index].sock = sock;
                        this->modbusTcpDevices[index].errMsg = "Connected successfully";
                        this->modbusTcpDevices[index].connStatu = true;
                    });
                }
            }
        }
    }
    std::vector<ModbusTcpDevice> ModbusTcp::getAllDeviceData()
    {
        return std::vector<ModbusTcpDevice>();
    }
    void ModbusTcp::updateDeviceData()
    {
        // 为每个设备创建一个任务，顺序读取所有字段
        for (size_t i = 0; i < this->modbusTcpDevices.size(); ++i) {
            size_t deviceIndex = i;
            this->tPool->submit([this, deviceIndex]()->void {
                while (true) {
                    {
                        std::lock_guard<std::mutex> lock(this->deviceMutex);
                        
                        // 检查设备是否有效且在线
                        if (deviceIndex >= this->modbusTcpDevices.size() || !this->modbusTcpDevices[deviceIndex].connStatu) {
                            return;
                        }
                        
                        ModbusTcpDevice& device = this->modbusTcpDevices[deviceIndex];
                        
                        // 顺序读取该设备的所有字段
                        for (auto& field : device.CollectionFields) {
                            if (!device.connStatu) {
                                break;
                            }
                            this->readRegister(device, field);
                        }
                    } // 锁在这里自动释放
                    
                    // 在锁外等待采集间隔
                    Sleep(this->CollectionInterval);
                }
            });
        }
    }
    ModbusTcpDevice ModbusTcp::getDeviceData(int did)
    {
        return ModbusTcpDevice();
    }
    // 读寄存器
    bool ModbusTcp::readRegister(ModbusTcpDevice& modbusTcpDevice, CollectionField& collectionField)
    {
        //构建请求 
        char headBuf[13] = { 0 };
        uint16_t transId = getNextTransactionId();
        headBuf[0] = (transId >> 8) & 0xFF;  // 事务ID高字节
        headBuf[1] = transId & 0xFF;         // 事务ID低字节
        headBuf[2] = 0x00;
        headBuf[3] = 0x00;
        // 长度字段：从站地址(1) + 功能码(1) + 寄存器地址(2) + 寄存器数量(2) = 6字节
        headBuf[4] = 0x00;
        headBuf[5] = 0x06;
        headBuf[6] = modbusTcpDevice.slaveId & 0xFF;  // 使用设备配置的从站ID
        headBuf[7] = 0x03;
        uint16_t addr = collectionField.fieldAddress;
        headBuf[8] = (addr >> 8) & 0xFF;
        headBuf[9] = addr & 0xFF;
        // registerLen 是字节数，每个寄存器2字节，所以寄存器数量 = registerLen / 2
        uint16_t registerCount = collectionField.registerLen / 2;
        headBuf[10] = (registerCount >> 8) & 0xFF;
        headBuf[11] = registerCount & 0xFF;
        
		int slen=send(modbusTcpDevice.sock, headBuf, 12, 0);
		if(slen<=0){
            // 修改设备状态（外部已加锁保护）
            modbusTcpDevice.sock = 0;
            modbusTcpDevice.connStatu = false;
            modbusTcpDevice.errMsg = "Message sending failed";
			return false;
		}else{
			//接收响应
			char recvBuf[100] = { 0 };
			int recvLen=recv(modbusTcpDevice.sock, recvBuf, 100, 0);
			if(recvLen<=0){
                // 修改设备状态（外部已加锁保护）
                modbusTcpDevice.sock = 0;
                modbusTcpDevice.connStatu = false;
                modbusTcpDevice.errMsg = "Message reception failed, connection has been disconnected";
				return false;
			}
            
            // 检查响应中的功能码是否正常（正常为0x03，异常为0x83）
            if (recvBuf[7] != 0x03) {
                if (recvBuf[7] == 0x83) {
                    // 异常响应
                    uint8_t exceptionCode = recvBuf[8];
                    modbusTcpDevice.errMsg = "Modbus exception response, code: " + std::to_string(exceptionCode);
                } else {
                    modbusTcpDevice.errMsg = "Unexpected function code in response: " + std::to_string(recvBuf[7]);
                }
                return false;
            }
            
            // 直接解析响应到当前字段（因为每个设备只有一个线程顺序处理）
            parseRegisterBuf(recvBuf, collectionField);
            modbusTcpDevice.errMsg = "Data received successfully";
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
    bool ModbusTcp::parseRegisterBuf(char* buff, CollectionField& collectionField)
	{
		int dataBitLen = 1;
        if(collectionField.buffType == BuffType::D_UINT8) {
            dataBitLen = 1;
            int len = buff[8] / dataBitLen;
            for (int i = 0; i < len; i++) {
                char bufData = buff[9 + i];
                collectionField.setData(bufData);
            }
        }
        else if (collectionField.buffType == BuffType::D_UINT16 || collectionField.buffType == BuffType::D_INT16) {
            dataBitLen = 2;
            int len = buff[8] / dataBitLen;
            for (int i = 0; i < len * 2; i += 2) {
                uint16_t bufData;
                switch (collectionField.byteSequence)
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
                if (collectionField.buffType == BuffType::D_UINT16) {
                    collectionField.setData(bufData);
                }
                else {
                    int16_t int16Data = static_cast<int16_t>(bufData);
                    collectionField.setData(int16Data);
                }
            }
        }
        else {
            dataBitLen = 4;
            int len = buff[8] / dataBitLen;
            for (int i = 0; i < len * 4; i += 4) {
                uint32_t bufData = 0;
                switch (collectionField.byteSequence)
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
                switch (collectionField.buffType)
                {
                case BuffType::D_UINT32:
                    collectionField.setData(bufData);
                    break;
                case BuffType::D_FLOAT:
                    float floatData; //转换成小数存储
                    memcpy(&floatData, &bufData, sizeof(floatData));
                    collectionField.setData(floatData);
                    break;
                case BuffType::D_INT64:
                    collectionField.setData(static_cast<uint64_t>(bufData));
                    break;
                case BuffType::D_UINT64:
                    collectionField.setData(static_cast<uint64_t>(bufData));
                    break;
                default: //BuffType::D_INT32
                    int32_t int32Data = static_cast<int32_t>(bufData);
                    collectionField.setData(int32Data);
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