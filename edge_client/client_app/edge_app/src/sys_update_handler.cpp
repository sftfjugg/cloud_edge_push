#include "sys_update_handler.h"
#include <iostream>
#include <fstream>
#include <cpr/cpr.h>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include "fileinfos_rw.h"
#include "loghelper.h"

using namespace RockLog;
namespace fs = boost::filesystem;
using namespace std;

SysUpdateHandler::SysUpdateHandler(/* args */)
{
}

SysUpdateHandler::~SysUpdateHandler()
{
}

void SysUpdateHandler::init(const std::string &fileServer)
{
    m_fileServer = fileServer;
    loadLocalFileInfos();
}

bool SysUpdateHandler::downloadFile(const std::string& url, const std::string& dst_filename)
{
    if (url.empty() || dst_filename.empty())
    {
        AMS_ERR("url or dst_filename is empty. url is {}, dst_filename is {}", url, dst_filename);
        return false;
    }
    static uint64_t s_totalSize = 0;
    boost::filesystem::path file_path = dst_filename;
    boost::filesystem::path parent_path = file_path.parent_path();
    if(!boost::filesystem::exists(parent_path))  
        boost::filesystem::create_directories(parent_path);   

    std::string tmp_filename = dst_filename + ".tmp";
    std::ofstream of(tmp_filename, std::ios::binary | std::ios::app);
    auto pos = of.tellp();
    std::string save_path = m_fileServer + "/" + url;

    AMS_INFO("save_path: {}, dst_filename: {}\n", save_path, dst_filename);

    cpr::Url cpr_url{save_path};
    cpr::Session s;
    s.SetVerifySsl(cpr::VerifySsl{false});
    s.SetUrl(cpr_url);
    //s.SetHeader(cpr::Header{{"Accept-Encoding", "gzip"}});
    
    auto fileLength = s.GetDownloadFileLength();
    s.SetRange(cpr::Range{pos, fileLength - 1});

    cpr::Response response = s.Download(of);
    s_totalSize += response.downloaded_bytes;
    
    AMS_INFO("GetDownloadFileLength: {}, response.status_code: {}, response.downloaded_bytes: {} \n", 
        fileLength, response.status_code, response.downloaded_bytes);
              
    if (s_totalSize >= fileLength)
    {
        s_totalSize = 0;
        rename(tmp_filename.c_str(), dst_filename.c_str());
        return true;
    }

    return false;
}

bool SysUpdateHandler::isNeedDownload(const FileInfo_t &t)
{
    bool find = false;
    for (auto f : m_fileInfo_vec)
    {
        if (f.filename == t.filename && f.save_path == t.save_path 
                && f.filesize == t.filesize && boost::iequals(f.md5, t.md5))
        {
            if (boost::filesystem::exists(f.save_path))
            {
                find = true;
                break;
            }
        }
    }
    return !find;
}
bool SysUpdateHandler::downloadFile(const FileInfo_t &t)
{
    if (downloadFile(t.remotefile_url, t.save_path))
    {
        FileInfo_t download_f;
        getFileInfo(t.save_path, download_f);

        if (!boost::iequals(download_f.md5, t.md5) || download_f.filesize != t.filesize)
        {
            AMS_ERR("download file {} missmatch with the source, download md5: {}, filesize: {}, src md5: {}, filesize: {}",  
                t.remotefile_url, download_f.md5.c_str(), download_f.filesize, t.md5.c_str(), t.filesize);
            // boost::filesystem::remove(t.save_path); // TODO
            return false;
        }
        else
        {
            AMS_INFO("download file {}, success! save to {}.", t.remotefile_url, t.save_path);
        }
        
        return true;
    }
    return false;
}

void SysUpdateHandler::loadLocalFileInfos()
{
    // fileInfos
    LOG(kInfo) << "loading file infos" ;
    const std::string fileInfosPath = boost::filesystem::current_path().string() + std::string("/file_infos.json");
    bool b = false;
    if (!boost::filesystem::exists(fileInfosPath))
    {
        AMS_ERR("file_infos.json not exist!");
        return;
    }

    b = loadFileInfos(fileInfosPath, m_fileInfo_vec);
    if (b)
        checkFileInfos(m_fileInfo_vec);

    for (auto f : m_fileInfo_vec)
        AMS_INFO("filename: {}, save_path: {} \n", f.filename, f.save_path);

}
void SysUpdateHandler::updateFileInfos(const FileInfo_t &t)
{
    bool find = false;
    for (auto i = 0; i < m_fileInfo_vec.size(); i++)
    {
        if (boost::iequals(m_fileInfo_vec[i].md5, t.md5)|| 
            (m_fileInfo_vec[i].filename == t.filename && m_fileInfo_vec[i].type == t.type) )
        {
            m_fileInfo_vec[i] = t;
            find = true;
            break;
        }
    }
    if (!find)
        m_fileInfo_vec.push_back(t);

    const std::string fileInfosPath = boost::filesystem::current_path().string() + std::string("/file_infos.json");
    writeFileInfos(fileInfosPath, m_fileInfo_vec);
}
