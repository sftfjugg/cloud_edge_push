# -*- coding: utf-8 -*-

import os
import requests
import time
import json

'''
struct FileInfo_t
{
    uint64_t filesize = 0;
    uint64_t modify_time = 0;       // 文件修改时间
    std::string type;               // 边端客户端类型，标识不同类型客户端
    std::string md5;                // 文件md5值
    std::string save_path;          // 保存至本地磁盘路径
    std::string remotefile_url;     // 从服务器下载文件路径
    std::string version;            // 文件版本
    std::string filename;           // 文件名
};
'''


class UploadTest:
    def __init__(self):
        self.srv_url = "http://192.21.1.61:9988"
        pass

    def uploadfiles(self):

        url = self.srv_url + '/sys/v1/service_upload'
        files = {'libunqlite.so': open(
            '../lib/libunqlite.so', 'rb'), 'http.log': open('../logs/http.log', 'rb')}
        data = [{"filename": "libunqlite.so", "save_path": "jack/libunqlite.so", "version": "1.0.1", "type": "SUZHOU"},
                {"filename": "http.log", "save_path": "rose/http.log", "version": "1.0.1", "type": "BEIJING"}]
        r = requests.post(url, data={"files_info": json.dumps(
            data)}, files=files, timeout=(10, 60))
        print(f'url: \n{url}\nrespone:\n{r.text}\n')


if __name__ == "__main__":
    api_test = UploadTest()
    api_test.uploadfiles()
