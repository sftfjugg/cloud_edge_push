#pragma once
#include <string>
#include <boost/property_tree/ptree.hpp>
#include "message_def.h"

// json message manager
class JsonMessageProcessor
{
public:
    JsonMessageProcessor() = default;
    ~JsonMessageProcessor() = default;

    std::string encodeHeartbeatReq(const HeartbeatReq_t& req);
    std::string encodeLoginReq(const LoginReq_t& req);
    std::string encodeSendFileInfosNtf(const SendFileInfosNtf_t &ntf);
    std::string encodeSysInfoNtf(const SendSysInfoNtf_t &ntf);

    bool decodeHeartbeatRsp(const std::string& json, HeartbeatRsp_t& rsp);
    bool decodeLoginRsp(const std::string& json, LoginRsp_t& rsp);
    bool decodeDownloadFileInfo(const std::string &json, FileInfo_t &t);

private:
    boost::property_tree::ptree _pt;
};

