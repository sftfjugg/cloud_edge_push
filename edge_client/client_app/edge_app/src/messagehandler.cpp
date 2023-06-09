#include <iostream>
#include "messagehandler.h"
#include "tcpclient.h"
#include "json_message_processor.h"
#include "loghelper.h"
#include "utility.hpp"
#include "call_async.hpp"
#include "messagebus.h"

using namespace boost;
using namespace std;
using boost::asio::ip::tcp;
using namespace RockLog;
static JsonMessageProcessor s_processor;

std::thread MessageHandler::start(const ClientConfig_t &cfg)
{
    _cfg = cfg;
    _tcpSessionStarted = false;
    std::thread t(&MessageHandler::startTcpSession, this);
    g_messagebus.attach<std::string>(kMsgFileInfos, &MessageHandler::onRecvFileInfos, *this);
    _sysUpdateHandler.init(cfg.httpServer);
    return t;
}

void MessageHandler::startTcpSession()
{
    TcpClient client(_io_context, _cfg);
    _tcpclient = &client;

    // add message handlers
    g_messagebus.attach<>(kMsgStart, &MessageHandler::onStart, *this);
    g_messagebus.attach<>(kMsgHeartbeatReq, &MessageHandler::onSendHeartbeatReq, *this);
    g_messagebus.attach<TcpMessage_t>(kMsgReadBody, &MessageHandler::onReadBody, *this);
    g_messagebus.attach<>(kMsgClose, &MessageHandler::onClose, *this);

    client.start();

    _io_context.run();
}

void MessageHandler::stop()
{
    _io_context.stop();
    _tcpclient = nullptr;
}

void MessageHandler::sendHeartbeatReq()
{
    if (_tcpclient == nullptr)
        return;

    LOG(kInfo) << "send heart_req";
    TcpMessage_t msg;
    HeartbeatReq_t req;
    req.ip = Utility::get_local_ip();
    msg.msgType = kHeartReq;
    msg.body = std::move(s_processor.encodeHeartbeatReq(req));
    if (_tcpSessionStarted)
        _tcpclient->write(msg);
}

void MessageHandler::sendLoginReq()
{
    if (_tcpclient == nullptr)
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
        _tcpclient->write(msg);

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

static std::string s_fileInfos; // 保存在本地文件中的已下载文件信息json串
void MessageHandler::onRecvFileInfos(std::string fileInfos)
{
    s_fileInfos = fileInfos;

    if (!s_fileInfos.empty() && _isLogin)
    {
        sendFileInfos();   // 发送已下载文件给服务器
        LOG(kInfo) << "s_fileInfos: " << s_fileInfos;
    }
}


void MessageHandler::sendFileInfos()
{
    if (_tcpclient == nullptr)
        return;

    LOG(kInfo) << "send FileInfos_ntf";
    TcpMessage_t msg;
    SendFileInfosNtf_t ntf;
    ntf.ip = Utility::get_local_ip();
    ntf.fileInfos = s_fileInfos;
    msg.msgType = kSendFileInfosNtf;
    msg.body = std::move(s_processor.encodeSendFileInfosNtf(ntf));
    LOG(kInfo) << "msg.body: " << msg.body;
    if (_tcpSessionStarted)
        _tcpclient->write(msg);
}



void MessageHandler::startSendFileInfosTimer()
{
    sendFileInfos();
    delay_call(60 * 1000, true, &MessageHandler::startSendFileInfosTimer, this);
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
        _isLogin = true;
        // 开始定时发送当前已下载文件信息
        startSendFileInfosTimer();
    }
    else if (kUpdateFileNtf == msg.msgType) // 获取到更新消息
    {
        LOG(kInfo) << "receive update file notify";
        FileInfo_t t;
        bool b = s_processor.decodeDownloadFileInfo(msg.body, t);
        if (!b)
        {
            LOG(kErr) << "decodeDownloadFileInfo err, msg.body: " << msg.body;
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
