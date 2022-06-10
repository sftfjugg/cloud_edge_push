
/*

| 消息       | 长度(BYTES)                               | 结构              | 描述                                         |
| ---------- | ----------------------------------------- | ----------------- | -------------------------------------------- |
| **Header** | 4                                         | SyncBytes         | 识别码，表示不同设备之间的交互               |
| **Header** | 4                                         | FullMessageLength | 数据总长度，包括SyncBytes和FullMessageLength |
| **Header** | 4                                         | MeaageType        | 消息类型（消息号)，如登录消息或发送消息消息  |
| **Header** | 4                                         | StringBody        | 消息字符流长度                               |
| **Body**   | StringBodyLength                          | StringBody        | 消息体长度，可能为0                          |
| **Body**   | FullMessageLength - 16 - StringBodyLength | BinaryData        | 二进制数据，长度可能为0                      |

*/
#pragma once

#include <string>
#include <vector>

struct TcpMessage_t
{
    uint8_t protoclHead[4] = {0x01, 0x01, 0x01, 0x01};
    uint32_t fullMsgLen = 0;
    uint32_t msgType = 0;
    uint32_t bodyLen = 0;
    std::string body;
    std::string binaryData;	// maybe empty
};


