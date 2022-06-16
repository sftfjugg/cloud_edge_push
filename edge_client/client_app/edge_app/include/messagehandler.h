#pragma once

#include <boost/asio.hpp>
#include <functional>
#include <atomic>
#include <string>
#include <map>
#include "message_def.h"
#include "tcp_message.h"
#include "syncwaitmsg.hpp"
#include "sys_update_handler.h"

class TcpClient;
class MessageHandler
{
public:
    static MessageHandler &instance()
    {
        static MessageHandler ins;
        return ins;
    }

    MessageHandler() = default;
    ~MessageHandler() = default;

    std::thread start(const ClientConfig_t &cfg);
    void startTcpSession();
    void stop();
    void sendHeartbeatReq();
    void sendLoginReq();
    void onTcpSessionError();

private:
    void onStart();
    void onClose();
    void onReadBody(TcpMessage_t msg);

    void onSendHeartbeatReq();
    void onRecvFileInfos(std::string fileInfos);
    void onRecvSysInfo(std::string sysInfo);

    void sendFileInfos();
    void sendSysInfo(const std::string &sysInfo);
    void startSendFileInfosTimer();

    SyncWaitMsg _syncObject;
    TcpClient *_tcpclient = nullptr;
    ClientConfig_t _cfg;
    boost::asio::io_context _io_context;
    std::atomic<bool> _tcpSessionStarted;
    std::atomic<bool> _isLogin;
    SysUpdateHandler _sysUpdateHandler;
};
