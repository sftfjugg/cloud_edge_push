#include <exception>
#include <iostream>
#include <boost/property_tree/json_parser.hpp>
#include "utility.hpp"
#include "json_message_processor.h"
#include "loghelper.h"

using namespace boost;
using namespace boost::property_tree;
using namespace std;
using namespace RockLog;

bool JsonMessageProcessor::decodeDownloadFileInfo(const std::string &json, FileInfo_t &t)
{
    try
    {
        LOG(kDebug) << "decodeDownloadFileInfo, body: " << std::endl
                    << json << std::endl;

        stringstream ss;
        ss << json;
        read_json(ss, _pt);

        auto f = _pt.get_child_optional("data");
        if (f)
        {
            auto modify_time = f->get_optional<uint64_t>("modify_time");
            if (modify_time)
                t.modify_time = *modify_time;

            auto type = f->get_optional<std::string>("type");
            if (type)
                t.type = *type;

            auto md5 = f->get_optional<std::string>("md5");
            if (md5)
                t.md5 = *md5;

            // 保存到本地的文件路径
            auto save_path = f->get_optional<std::string>("save_path");
            if (save_path)
                t.save_path = *save_path;

            // 对应的工作的物理文件路径
            auto remotefile_url = f->get_optional<std::string>("remotefile_url");
            if (remotefile_url)
                t.remotefile_url = *remotefile_url;

            auto version = f->get_optional<std::string>("version");
            if (version)
                t.version = *version;

            auto filename = f->get_optional<std::string>("filename");
            if (filename)
                t.filename = *filename;

            auto filesize = f->get_optional<uint64_t>("filesize");
            if (filesize)
                t.filesize = *filesize;
                
            return true;
        }
    }
    catch (std::exception &e)
    {
        LOG(kErr) << "[E] decodeUpServerCfgNtf: unknow error: " << e.what() << endl;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////

std::string JsonMessageProcessor::encodeHeartbeatReq(const HeartbeatReq_t &req)
{
    ptree pt, ptData;
    pt.put<std::string>("msg_type", "heart_req");
    pt.put<uint64_t>("timestamp", Utility::getTimeStamp());
    ptData.put("ip", req.ip);

    pt.put_child("data", ptData);

    ostringstream ss;
    write_json(ss, pt);
    return ss.str();
}

std::string JsonMessageProcessor::encodeLoginReq(const LoginReq_t &req)
{
    ptree pt, ptData;
    pt.put<std::string>("msg_type", "login_req");
    pt.put<uint64_t>("timestamp", Utility::getTimeStamp());

    ptData.put<std::string>("ip", req.ip);
    ptData.put<std::string>("type", req.type);

    pt.put_child("data", ptData);

    ostringstream ss;
    write_json(ss, pt);
    return ss.str();
}

std::string JsonMessageProcessor::encodeSendFileInfosNtf(const SendFileInfosNtf_t &ntf)
{
    ptree pt, ptData;
    pt.put<std::string>("msg_type", "file_infos_ntf");
    pt.put<uint64_t>("timestamp", Utility::getTimeStamp());

    ptData.put<std::string>("ip", ntf.ip);
    ptData.put<std::string>("file_infos", ntf.fileInfos);

    pt.put_child("data", ptData);

    ostringstream ss;
    write_json(ss, pt);
    return ss.str();
}



std::string JsonMessageProcessor::encodeSysInfoNtf(const SendSysInfoNtf_t &ntf)
{
    ptree pt, ptData;
    pt.put<std::string>("msg_type", "file_infos_ntf");
    pt.put<uint64_t>("timestamp", Utility::getTimeStamp());

    ptData.put<std::string>("ip", ntf.ip);
    ptData.put<std::string>("sys_info", ntf.sysinfo);

    pt.put_child("data", ptData);

    ostringstream ss;
    write_json(ss, pt);
    return ss.str();
}


