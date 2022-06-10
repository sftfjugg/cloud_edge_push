#pragma once

#include <boost/asio.hpp>
#include <functional>
#include <atomic>
#include <string>
#include <map>
#include "message_def.h"
#include "tcp_message.h"
#include "syncwaitmsg.hpp"
#include "thread_pool.hpp"
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
    void onSendHeartbeatReq();
    void onReadBody(TcpMessage_t msg);

    SyncWaitMsg _syncObject;
    TcpClient *_client = nullptr;
    ClientConfig_t _cfg;
    boost::asio::io_context _io_context;
    std::atomic<bool> _tcpSessionStarted;
    SysUpdateHandler _sysUpdateHandler;
};
