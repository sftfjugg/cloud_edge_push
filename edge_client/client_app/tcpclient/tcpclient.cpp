
#include "tcpclient.h"
#include "tcpclient_impl.h"
#include <iostream>
TcpClient::TcpClient(boost::asio::io_context& io_context, const ClientConfig_t& cfg)
    :_pImpl(new TcpClientImpl(io_context, cfg))
{
}

TcpClient::~TcpClient()
{
    delete _pImpl;
}

void TcpClient::start()
{
    return _pImpl->start();
}

void TcpClient::write(TcpMessage_t& msg)
{
    return _pImpl->write(msg);
}

void TcpClient::close()
{
    return _pImpl->close();
}

MessageBus<>& TcpClient::messagebus()
{
    return _pImpl->messagebus();
}
