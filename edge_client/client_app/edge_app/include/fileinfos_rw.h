#pragma once
#include <string>
#include <vector>
#include "message_def.h"

/*
读取或者保存已下载的文件信息
一般在程序初始化的时候读取一次，
而在每次下载文件后，都要保存一次已下载文件
*/

bool readFile2Str(const std::string &filepath, std::string &fileInfo);
bool loadFileInfosFromStr(const std::string &fileInfo, std::vector<FileInfo_t> &m_fileInfo_vec);
bool loadFileInfos(const std::string &filepath, std::vector<FileInfo_t> &m_fileInfo_vec);
bool checkFileInfos(std::vector<FileInfo_t> &m_fileInfo_vec);
bool getFileInfo(const std::string filePath, FileInfo_t &t);
void writeFileInfos(const std::string &filepath, const std::vector<FileInfo_t> &fileInfo_vec);
void writeFileInfos2File(const std::string &filepath, const std::string &fileInfos);
std::string writeFileInfos2Str(const std::vector<FileInfo_t> &fileInfo_vec);