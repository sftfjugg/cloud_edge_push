#include "fileinfos_rw.h"
#include "app_common.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/algorithm/string.hpp>
#include <fstream>
#include <boost/algorithm/hex.hpp>
#include <boost/uuid/detail/md5.hpp>
#include <boost/filesystem.hpp>
#include <iostream>
#include "loghelper.h"

using namespace RockLog;
using namespace boost;

// load app configure
bool loadFileInfos(const std::string &filepath, std::vector<FileInfo_t> &m_fileInfo_vec)
{
    try
    {
        if (boost::filesystem::exists(filepath))
        {
            property_tree::ptree pt;
            property_tree::json_parser::read_json(filepath, pt);

            auto files = pt.get_child_optional("files");
            if (files)
            {
                for (auto it : *files)
                {
                    auto f = it.second;
                    FileInfo_t t;
                    auto modify_time = f.get_optional<uint64_t>("modify_time");
                    if (modify_time)
                        t.modify_time = *modify_time;

                    auto type = f.get_optional<int>("type");
                    if (type)
                        t.type = *type;

                    auto md5 = f.get_optional<std::string>("md5");
                    if (md5)
                        t.md5 = *md5;

                    // 保存至本地磁盘路径
                    auto save_path = f.get_optional<std::string>("save_path");
                    if (save_path)
                        t.save_path = *save_path;

                    // 从服务器下载文件路径
                    auto remotefile_url = f.get_optional<std::string>("remotefile_url");
                    if (remotefile_url)
                        t.remotefile_url = *remotefile_url;

                    auto version = f.get_optional<std::string>("version");
                    if (version)
                        t.version = *version;

                    auto filename = f.get_optional<std::string>("filename");
                    if (filename)
                        t.filename = *filename;

                    auto filesize = f.get_optional<uint64_t>("filesize");
                    if (filesize)
                        t.filesize = *filesize;
                    m_fileInfo_vec.push_back(t);
                }
                return true;
            }
        }
        return false;
    }
    catch (const std::exception &e)
    {
        std::cout << e.what() << std::endl;
        return false;
    }
}

void writeFileInfos(const std::string &filepath, const std::vector<FileInfo_t> &fileInfo_vec)
{
    try
    {
        property_tree::ptree pt;
        property_tree::ptree files;
        for (auto f : fileInfo_vec)
        {
            property_tree::ptree child;

            child.put<uint64_t>("modify_time", f.modify_time);
            child.put<std::string>("type", f.type);
            child.put<std::string>("md5", f.md5);
            child.put<std::string>("save_path", f.save_path);
            child.put<std::string>("remotefile_url", f.remotefile_url);
            child.put<std::string>("version", f.version);
            child.put<std::string>("filename", f.filename);
            child.put<uint64_t>("filesize", f.filesize);
            files.push_back(std::make_pair("", child));
        }
        pt.add_child("files", files);
        boost::filesystem::remove(filepath);
        write_json(filepath, pt);
    }
    catch (const std::exception &e)
    {
        LOG(kErr) << e.what() << '\n';
    }
}

static bool getMd5(std::string &str_md5, const char *const buffer, size_t buffer_size)
{
    if (buffer == 0)
        return false;
    boost::uuids::detail::md5 boost_md5;
    boost_md5.process_bytes(buffer, buffer_size);
    boost::uuids::detail::md5::digest_type digest;
    boost_md5.get_digest(digest);
    const auto char_digest = reinterpret_cast<const char *>(&digest);
    str_md5.clear();
    boost::algorithm::hex(char_digest, char_digest + sizeof(boost::uuids::detail::md5::digest_type), std::back_inserter(str_md5));
    return true;
}

static std::string getMd5(const std::string &filename)
{
    std::string str_md5;
    std::ifstream is(filename, std::ifstream::binary);
    if (is)
    {
        // get length of file:
        is.seekg(0, is.end);
        int length = is.tellg();
        is.seekg(0, is.beg);

        // allocate memory:
        char *buffer = new char[length];
        // read data as a block:
        is.read(buffer, length);
        is.close();

        getMd5(str_md5, buffer, length);
        delete[] buffer;
    }
    return str_md5;
}

bool getFileInfo(const std::string filePath, FileInfo_t &t)
{
    boost::filesystem::path p(filePath);
    if (!boost::filesystem::exists(p))
        return false;
    t.filesize = boost::filesystem::file_size(p);
    t.md5 = getMd5(filePath);
    t.modify_time = boost::filesystem::last_write_time(p);
    return true;
}

bool checkFileInfos(std::vector<FileInfo_t> &m_fileInfo_vec)
{
    try
    {
        for (auto iter = m_fileInfo_vec.begin(); iter != m_fileInfo_vec.end();)
        {
            auto filePath = iter->remotefile_url;
            FileInfo_t f;
            bool ok = true;
            if (getFileInfo(filePath, f))
            {
                if (f.filesize != iter->filesize || !boost::iequals(f.md5, iter->md5))
                    ok = false;
            }
            else
                ok = false;

            if (!ok)
                m_fileInfo_vec.erase(iter++);
            else
                ++iter;
        }
    }
    catch (const std::exception &e)
    {
        std::cout << e.what() << std::endl;
        return false;
    }
}
