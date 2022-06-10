
#include "loghelper.h"
#include <stdint.h>
#include <fstream> // std::ifstream, std::ofstream
#include <iostream>
#include <boost/filesystem.hpp>
#include <mutex> // std::mutex, std::unique_lock
#include "concurrentqueue.h" // https://github.com/cameron314/concurrentqueue

using namespace RockLog;
using namespace std;
using namespace moodycamel;

static std::mutex s_mtx; // mutex for critical section
static bool s_isConcurrentQueueStarted = false; // 暂不加锁

struct Item_t
{
    std::string filename;
    std::string content;
};

static ConcurrentQueue<Item_t> s_itemQueue;

Log2File::Log2File(std::string filename)
    : _filename(filename)
{
}


static void checkAndCreateFile(const std::string& filename)
{
    try
    {
        if (!boost::filesystem::exists(filename))
        { //判断路径是否存在
            boost::filesystem::path p(filename);
            if (!boost::filesystem::exists(p.parent_path()))
                boost::filesystem::create_directories(p.parent_path()); //文件夹不存在则创建一个
        }
    }
    catch (const std::exception &e)
    {
        std::cout << e.what() << std::endl;
        return;
    }
}

// 创建单独线程，从消息队列中取消息写入文件。
void Log2File::startConsumeThread()
{
    std::thread([&]() {
        while (true)
        {
            try
            {
                Item_t item;
                if (s_itemQueue.try_dequeue(item))
                {
                    checkAndCreateFile(item.filename);
                    std::ofstream outfile;
                    outfile.open(item.filename, ios::app | ios::binary);
                    outfile << item.content;
                    outfile.close();
                }
            }
            catch(const std::exception& e)
            {
                s_isConcurrentQueueStarted = false;
                std::cerr << e.what() << '\n';
                break;
            }
        }
        s_isConcurrentQueueStarted = false;
    }).detach();
    s_isConcurrentQueueStarted = true;
}

Log2File::~Log2File()
{
    if (!s_isConcurrentQueueStarted)
    {
        checkAndCreateFile(_filename);
        std::unique_lock<std::mutex> lck(s_mtx);
        std::ofstream outfile;
        outfile.open(_filename, ios::app | ios::binary);
        outfile << _ss.str();
        //std::cout << _ss.str() << std::endl;
        outfile.close();
    }
    else
    {
        Item_t item;
        item.content = _ss.str();
        item.filename =_filename;
        s_itemQueue.enqueue(item);
    }

}
