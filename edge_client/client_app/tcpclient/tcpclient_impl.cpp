#include "tcpclient_impl.h"
#include <iostream>
#include "bytes_buffer.hpp"
#include "app_common.h"
#include "loghelper.h"
#include <thread>
#include <chrono>

using namespace RockLog;
using namespace boost;
using boost::asio::deadline_timer;
using boost::asio::ip::tcp;

void TcpClientImpl::write(TcpMessage_t &msg)
{
    if (_stopped)
        return;

    memcpy(msg.protoclHead, kProtoclHead, 4);
    msg.bodyLen = msg.body.length();
    msg.fullMsgLen = kTotalHeadLen + msg.bodyLen;
    _io_context.post(std::bind(&TcpClientImpl::startWrite, this, msg));
}

void TcpClientImpl::close()
{
    if (!_closed)
    {
        LOG(kErr) << "TcpClientImpl call close!" << std::endl;
        _io_context.post(std::bind(&TcpClientImpl::doClose, this));
        _closed = true;
    }
}

//----- private functions -----

void TcpClientImpl::start()
{
    LOG(kInfo) << "Trying " << _endpoints->endpoint() << "...\n";

    // Start the asynchronous connect operation.
    boost::asio::async_connect(_socket, _endpoints,
        std::bind(&TcpClientImpl::handleConnect, this, std::placeholders::_1));
}

void TcpClientImpl::handleConnect(const boost::system::error_code &ec)
{
    if (ec || !_socket.is_open())
    {
        if (ec)
        {
            LOG(kErr) << "Connected to server " << _endpoints->endpoint() << " failed!\n";
            _socket.close();
        }
        else
            LOG(kErr) << "Connected to server " << _endpoints->endpoint() << " timeout!\n";

        _stopped = true;

        if (!_clientConfig.isReconnect)
        {
            //throw boost::system::system_error(ec ? ec : boost::asio::error::operation_aborted);
            close();
            _messagebus.sendMessage(kMsgTcpSessionError);
        }
        else
        {
            close();
            startReconnect();
        }
        return;
    }
    // Otherwise we have successfully established a connection.
    else
    {
        _stopped = false;
        _closed = false;
        _reconnectFlag = 0;

        LOG(kInfo) << "Connected to " << _endpoints->endpoint() << "\n";
        
        // program start
        _messagebus.sendMessage(kMsgStart);

        // Start the input actor.
        startRead();

        _heartbeatTimer.expires_from_now(boost::posix_time::seconds(_clientConfig.heartbeatInterval));
        _heartbeatTimer.async_wait(std::bind(&TcpClientImpl::checkHeartbeat, this));
        _isRecvedHeartbeatRsp = false;
    }
}

void TcpClientImpl::doReconnect(const boost::system::error_code &ec)
{
    LOG(kInfo) << "reconnect to server " << _endpoints->endpoint() << "......"
        << "\n";
    // Start the asynchronous connect operation.
    boost::asio::async_connect(_socket, _endpoints,
        std::bind(&TcpClientImpl::handleConnect, this, std::placeholders::_1));
}

void TcpClientImpl::startRead()
{
    // Start an asynchronous operation to read a message.
    _rbuf.clear();
    _rbuf.resize(kTotalHeadLen);
    boost::asio::async_read(_socket,
        boost::asio::buffer((char *)_rbuf.c_str(), kTotalHeadLen),
        std::bind(&TcpClientImpl::handleReadHeader, this, std::placeholders::_1));
}

void TcpClientImpl::handleReadHeader(const boost::system::error_code &ec)
{
    if (_stopped)
        return;
    
    int headerError = 0;
    if (!ec)
    {
        BytesBuffer buf;
        buf.append(_rbuf.data(), _rbuf.length());

        buf.retrieve((char *)&_readMsg.protoclHead, kIntLen);
        if (0 != memcmp(kProtoclHead, _readMsg.protoclHead, kIntLen))
        {
            LOG(kErr) << "!!! error sync_bytes! protoclHead: " << _readMsg.protoclHead << std::endl;
            headerError = 1;
        }

        buf.retrieve((char *)&_readMsg.fullMsgLen, kIntLen);
        if (kMaxMessageSize < _readMsg.fullMsgLen)
        {
            LOG(kErr) << "!!! message too big! fullMsgLen: " << _readMsg.fullMsgLen << std::endl;
            headerError = 1;
        }

        buf.retrieve((char *)&_readMsg.msgType, 4); 

        // reset read buffer size
        _rbuf.resize(_readMsg.fullMsgLen);

        boost::asio::async_read(_socket,
            boost::asio::buffer((char *)_rbuf.c_str() + kTotalHeadLen, _readMsg.fullMsgLen - kTotalHeadLen),
            std::bind(&TcpClientImpl::handleReadBody, this, std::placeholders::_1));

        // received heartbreat respone
        if (kHeartRsp == _readMsg.msgType)
        {
            _isRecvedHeartbeatRsp = true;
            LOG(kDebug) << "Set the heartbeat flag to true, when receive heartbeat response" << std::endl;
        }
        
        _messagebus.sendMessage(kMsgReadHeader, _readMsg);
    }
    else
    {
        LOG(kErr) << "!!! disconneted with server! ec.message: " << ec.message() << ";"
            << "error code is: " << ec << std::endl;
        headerError = 1;
    }

    if(headerError)
    {
        close();
        _stopped = true;
        if (_clientConfig.isReconnect)
        {
            startReconnect();
        }
    }
}

void TcpClientImpl::handleReadBody(const boost::system::error_code &ec)
{
    if (_stopped)
        return;

    if (!ec)
    {
        BytesBuffer buf;
        buf.append(_rbuf.c_str() + kTotalHeadLen, _readMsg.fullMsgLen - kTotalHeadLen);

        _readMsg.bodyLen = _readMsg.fullMsgLen - kTotalHeadLen;
        if (_readMsg.bodyLen > 0)
        {
            auto body = buf.retrieveAsString(_readMsg.bodyLen);
            _readMsg.body = body;
        }

        _messagebus.sendMessage(kMsgReadBody, _readMsg);

        _rbuf.clear();
        _rbuf.resize(kTotalHeadLen);

        boost::asio::async_read(_socket,
            boost::asio::buffer((char *)_rbuf.c_str(), kTotalHeadLen),
            std::bind(&TcpClientImpl::handleReadHeader, this, std::placeholders::_1));
    }
    else
    {
        LOG(kErr) << "Error on handleReadBody: " << ec.message() << "\n";
        close();
        _stopped = true;
        if (_clientConfig.isReconnect)
        {
            startReconnect();
        }
    }
}

void TcpClientImpl::doWrite(TcpMessage_t &msg)
{
    BytesBuffer buf;

    buf.append((char *)&msg.protoclHead, 4);
    buf.append((char *)&msg.fullMsgLen, 4);
    buf.append((char*)&msg.msgType, 4);
    buf.append((char *)&msg.bodyLen, 4);
    buf.append(msg.body);

    _wbuf.clear();
    _wbuf.resize(msg.fullMsgLen);
    std::copy(buf.begin(), buf.begin() + buf.readableBytes(), _wbuf.begin());

    // Start an asynchronous operation to send a message.
    boost::asio::async_write(_socket,
        boost::asio::buffer((char *)_wbuf.c_str(), _wbuf.length()),
        std::bind(&TcpClientImpl::handleWrite, this, std::placeholders::_1));
}

void TcpClientImpl::startWrite(const TcpMessage_t &msg)
{
    if (_stopped)
        return;

    bool write_in_progress = !_writeMsgs.empty();
    _writeMsgs.push_back(msg);
    if (!write_in_progress)
    {
        auto &front = _writeMsgs.front();
        doWrite(front);
    }
}

void TcpClientImpl::handleWrite(const boost::system::error_code &ec)
{
    if (_stopped)
        return;

    if (!ec)
    {
        _writeMsgs.pop_front();
        if (!_writeMsgs.empty())
        {
            auto &front = _writeMsgs.front();
            doWrite(front);
        }
    }
    else
    {
        LOG(kErr) << "Error on write: " << ec.message() << "\n";
        close();
        _stopped = true;
        if (_clientConfig.isReconnect)
        {
            startReconnect();
        }
    }
}

void TcpClientImpl::checkHeartbeat()
{
    if (_stopped)
        return;

    if (_heartbeatTimer.expires_at() <= deadline_timer::traits_type::now())
    {
        if (_isRecvedHeartbeatRsp)
        {
            // send heartbeat message
            _messagebus.sendMessage(kMsgHeartbeatReq);
            _isRecvedHeartbeatRsp = false;
            _heartBeatLostTimes = 0; // re-init
        }
        else
        {
            // heartbeat timeout

            if (_heartBeatLostTimes >= 3) // lost three times
            {
                LOG(kErr) << "heart beat timeout three times, disconnect!" << std::endl;
                close();
                _heartbeatTimer.expires_at(boost::posix_time::pos_infin);
                startReconnect();
            }
            else
            {
                _heartBeatLostTimes += 1;
                _messagebus.sendMessage(kMsgHeartbeatReq);
            }
        }
        _heartbeatTimer.expires_from_now(boost::posix_time::seconds(_clientConfig.heartbeatInterval));
    }

    // Put the actor back to sleep.
    _heartbeatTimer.async_wait(std::bind(&TcpClientImpl::checkHeartbeat, this));
}

// This function terminates all the actors to shut down the connection. It
// may be called by the user of the client class, or by the class itself in
// response to graceful termination or an unrecoverable error.
void TcpClientImpl::doClose()
{
    LOG(kDebug) << "TcpClientImpl:doClose()" << std::endl;
    _messagebus.sendMessage(kMsgClose);
    _stopped = true;
    boost::system::error_code ignored_ec;
    _socket.close(ignored_ec);
    _writeMsgs.clear();
    _heartBeatLostTimes = 0;
    _heartbeatTimer.cancel();
}

void TcpClientImpl::startReconnect()
{
    if(_reconnectFlag == 0)
    {
        // schedule a timer to reconnect after 5 seconds
        _reconnectTimer.expires_from_now(boost::posix_time::seconds(_clientConfig.reconnectInterval));
        _reconnectTimer.async_wait(std::bind(&TcpClientImpl::doReconnect, this, std::placeholders::_1));
    }
    else
    {    
        if (_reconnectTimer.expires_at() <= deadline_timer::traits_type::now())
        {
            _reconnectTimer.expires_from_now(boost::posix_time::seconds(_clientConfig.reconnectInterval));
            _reconnectTimer.async_wait(std::bind(&TcpClientImpl::doReconnect, this, std::placeholders::_1));
        }
    }
    _reconnectFlag++;
}
