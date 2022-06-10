#pragma once
#include <stdint.h>
#include <string>
#include <map>
#include <functional>

#include "message_def.h"

static const uint32_t kIntLen         = 4;                 // sizeof(int) is 4
static const uint32_t kTotalHeadLen   = 16;                // head of TcpMessage is 4*8
static const uint32_t kMaxBodySize    = 1024 * 10;         // Max TcpMessage body size
static const uint32_t kMaxMessageSize = 1024 * 1024 * 64;  // Max TcpMessage size

static const char* kMsgStart           = "handleStart";            // first thing todo when connected
static const char* kMsgTcpSessionError = "handleTcpSessionError";  // tcp session error, eg. connected failed
static const char* kMsgClose           = "handleClose";            // clear when tcpsession closed
static const char* kMsgHeartbeatReq    = "sendHeartbeatReq";       // send heartbeat_req 
static const char* kMsgReadHeader      = "handleReadHeader";       // MessageBus register topic for sending read header to messagehandler module
static const char* kMsgReadBody        = "handleReadBody";         // essageBus register topic for sending read body to messagehandler module

enum CloseError_t
{
    Client_OK = 0,
    SyncBytes_Error,
    MsgLen_Error,
    SeqNum_Error,
    BodyLen_Error,
    Write_Error,
    HeartbeatExpire_Error,
    HandleHeader_Exception,
    HandleBody_Exception,
    Socket_Disconnect
};
