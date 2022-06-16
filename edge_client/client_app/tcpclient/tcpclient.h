
#pragma once

#include <boost/asio.hpp>
#include "tcp_message.h"
#include "app_common.h"

#if (!defined(ROCK_LIB) && !defined(ROCK_LINUX_PLATFORM))
// ROCK is used as DLL
#define ROCK_DLLEXPORT __declspec(dllexport) 
#define ROCK_DLLIMPORT __declspec(dllimport) 
#else 
#define ROCK_DLLEXPORT 
#define ROCK_DLLIMPORT
#endif

#ifdef ROCK_NET_EXPORTS
#define ROCK_NET_API  ROCK_DLLEXPORT  
#else 
#define ROCK_NET_API  ROCK_DLLIMPORT 
#endif


class TcpClientImpl;
class ROCK_NET_API TcpClient
{
public:
    TcpClient(boost::asio::io_context& io_context,
        const ClientConfig_t &cfg);
    ~TcpClient();
    // note: _syncBytes _bodyLen _fullMsgLen这三个字段会自动补全，可以不用填写
    void write(TcpMessage_t &msg);
    void close();
    void start();

private:

    TcpClientImpl* _pImpl;
};


