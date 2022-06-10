#pragma once

#include <string>

static const uint8_t  kProtoclHead[4]  = {0x01,0x01,0x01,0x01};

static const uint32_t kLoginReq = 2001;
static const uint32_t kLoginRsp = 2002;

static const uint32_t kHeartReq = 2011;
static const uint32_t kHeartRsp = 2012;

static const uint32_t kUpdateFileNtf = 2023;


struct ClientConfig_t
{
    bool isReconnect = false;
    int reconnectInterval = 5;
    int heartbeatInterval = 10;
    std::string serverIP;
    std::string serverPort;
    std::string httpServer;
    std::string clientType;
};

struct HeartbeatReq_t
{
    std::string ip;
};

struct HeartbeatRsp_t
{
    int code;
    std::string message;
};


struct LoginReq_t
{
    std::string ip;
    std::string type;
};

struct LoginRsp_t
{
    int code;
    std::string message;
};

enum ResponseCode_t
{
    CODE_SUCCESS = 1000,
    CODE_FAIL = 1007
};


struct FileInfo_t
{
    uint64_t filesize = 0;
    uint64_t modify_time = 0;       // 文件修改时间
    std::string type;               // 边端客户端类型，标识不同类型客户端
    std::string md5;                // 文件md5值
    std::string save_path;          // 保存至本地磁盘路径
    std::string remotefile_url;     // 从服务器下载文件路径
    std::string version;            // 文件版本
    std::string filename;           // 文件名
};