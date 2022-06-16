
#pragma once

#include <deque>
#include <boost/asio.hpp>
#include <atomic>
#include "tcp_message.h"
#include <stdint.h>
#include "app_common.h"


class TcpClientImpl
{
public:
    TcpClientImpl(boost::asio::io_context &io_context, const ClientConfig_t &cfg)
        : _io_context(io_context),
        _socket(io_context),
        _heartbeatTimer(io_context),
        _reconnectTimer(io_context),
        _clientConfig(cfg),
        _stopped(true),
        _closed(false),
        _reconnectFlag(0)
    {
        boost::asio::ip::tcp::resolver resolver(io_context);
        _endpoints = resolver.resolve(_clientConfig.serverIP, _clientConfig.serverPort);
    }

    ~TcpClientImpl() { close(); }
    void write(TcpMessage_t& msg);

    void close();

    void start();

    bool stopped() { return _stopped; }

private:

    void handleConnect(const boost::system::error_code& ec);

    void doReconnect(const boost::system::error_code& ec);

    void startRead();

    void handleReadHeader(const boost::system::error_code& ec);

    void handleReadBody(const boost::system::error_code& ec);

    void doWrite(TcpMessage_t& msg);

    void startWrite(const TcpMessage_t& msg);

    void handleWrite(const boost::system::error_code& ec);

    void checkHeartbeat();

    void doClose();

    void startReconnect();

private:

    ClientConfig_t _clientConfig;
    std::atomic<bool> _stopped;
    std::atomic<bool> _closed;    // check if tcp session closed
    std::atomic<int> _reconnectFlag;
    boost::asio::io_context& _io_context;
    boost::asio::ip::tcp::socket _socket;
    boost::asio::deadline_timer _heartbeatTimer;
    boost::asio::deadline_timer _reconnectTimer;

    TcpMessage_t _readMsg;
    std::deque<TcpMessage_t> _writeMsgs;

    bool _isRecvedHeartbeatRsp = false;

    uint32_t _heartBeatLostTimes = 0;

    std::string _rbuf;    // message bytes read buffer
    std::string _wbuf;    // message bytes write buffer

    boost::asio::ip::tcp::resolver::results_type _endpoints;
};

