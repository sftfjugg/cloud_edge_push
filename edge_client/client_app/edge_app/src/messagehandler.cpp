#include <iostream>
#include "messagehandler.h"
#include "tcpclient.h"
#include "json_message_processor.h"
#include "loghelper.h"
#include "utility.hpp"

using namespace boost;
using namespace std;
using boost::asio::ip::tcp;
using namespace RockLog;
static JsonMessageProcessor s_processor;


std::thread MessageHandler::start(const ClientConfig_t& cfg)
{
    _cfg = cfg;
    _tcpSessionStarted = false;
    std::thread t(&MessageHandler::startTcpSession, this);

    _sysUpdateHandler.init(cfg.httpServer);
    return t;
}

void MessageHandler::startTcpSession()
{
    TcpClient client(_io_context, _cfg);
    _client = &client;

    // add message handlers
    client.messagebus().attach<>(kMsgStart, &MessageHandler::onStart, *this);
    client.messagebus().attach<>(kMsgHeartbeatReq, &MessageHandler::onSendHeartbeatReq, *this);
    client.messagebus().attach<TcpMessage_t>(kMsgReadBody, &MessageHandler::onReadBody, *this);
    client.messagebus().attach<>(kMsgClose, &MessageHandler::onClose, *this);

    client.start();

    _io_context.run();
}

void MessageHandler::stop()
{
    _io_context.stop();
    _client = nullptr;
}

void MessageHandler::sendHeartbeatReq()
{
    if (_client == nullptr)
        return;

    LOG(kInfo) << "send heart_req";
    TcpMessage_t msg;
    HeartbeatReq_t req;
    req.ip = Utility::get_local_ip();
    msg.msgType = kHeartReq;
    msg.body = std::move(s_processor.encodeHeartbeatReq(req));
    if (_tcpSessionStarted)
        _client->write(msg);
}

void MessageHandler::sendLoginReq()
{
    if (_client == nullptr)
        return;

    LOG(kInfo) << "send login_req";
    // TODO 登录的时候将edge客户端相关信息发生至服务端
    TcpMessage_t msg;
    LoginReq_t req;
    req.ip = Utility::get_local_ip();
    req.type = _cfg.clientType;
    
    msg.msgType = kLoginReq;
    msg.body = std::move(s_processor.encodeLoginReq(req));
    if (_tcpSessionStarted)
        _client->write(msg);

    _syncObject.wait(kLoginRsp, 3); // 等待3秒，一般必须收到login_rsp消息才能进行下一步处理业务
}

void MessageHandler::onTcpSessionError()
{
    LOG(kErr) << "[MessageHandler::onTcpSessionError]" << std::endl;
    stop();
}

//--------------------- private functions -----------------------

void MessageHandler::onStart()
{
    _tcpSessionStarted = true;
    sendHeartbeatReq();
}

void MessageHandler::onClose()
{
    _tcpSessionStarted = false;
}

void MessageHandler::onSendHeartbeatReq()
{
    sendHeartbeatReq();
}



void MessageHandler::onReadBody(TcpMessage_t msg)
{
    static bool sent = false;
    if (kHeartRsp == msg.msgType)
    {
        LOG(kInfo) << "receive heart_rsp";
        if (!sent)
        {
            sent = true;
            sendLoginReq();
        }
    }
    else if (kLoginRsp == msg.msgType)
    {
        LOG(kInfo) << "receive login_rsp";
        _syncObject.receive(msg.msgType); 
    }
    else if (kUpdateFileNtf == msg.msgType) // 获取到更新消息
    {
        LOG(kInfo) << "receive update file notify";
        FileInfo_t t;
        bool b = s_processor.decodeDownloadFileInfo(msg.body, t);
        if (!b)
        {
            LOG(kErr) << "decodeDownloadFileInfo err, msg.body: " << msg.body ;
            return;
        }
        if (!_sysUpdateHandler.isNeedDownload(t))
        {
            LOG(kInfo) << t.remotefile_url << " already exist, skip!";
            return;
        }

        bool ok = _sysUpdateHandler.downloadFile(t); // 需要组装file_url
        if (ok)
        {
            LOG(kInfo) << "downloadFile " << t.remotefile_url << " success.";
            _sysUpdateHandler.updateFileInfos(t);
        }
        else
            LOG(kErr) << "downloadFile " << t.remotefile_url << " failed!!!";
    }
    else
    {
        LOG(kInfo) << "receive msgType: " << msg.msgType << "\n";
    }
}

