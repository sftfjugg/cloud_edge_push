#pragma once
#include <string>
#include <vector>
#include "message_def.h"

/*
系统更新下载文件处理业务
*/
class SysUpdateHandler
{

public:
    SysUpdateHandler(/* args */);
    ~SysUpdateHandler();
    void init(const std::string &fileServer);
    bool downloadFile(const std::string& url, const std::string& filename);
    bool downloadFile(const FileInfo_t &t);
    void updateFileInfos(const FileInfo_t &t);
    void loadLocalFileInfos();  
    bool isNeedDownload(const FileInfo_t &t);

private:
    std::string m_fileServer;
    std::vector<FileInfo_t> m_fileInfo_vec;
};

