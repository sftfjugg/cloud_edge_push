#include "loghelper.h"
using namespace RockLog;

// ref: http://www.cplusplus.com/reference/thread/thread/?kw=thread
// thread example
#include <iostream>       // std::cout
#include <thread>         // std::thread

#define msleep(x) std::this_thread::sleep_for(std::chrono::milliseconds(x))
void foo()
{
    std::string tag = "FOO";
    LogHelper::initLogHelper(tag);
    // do stuff...
    LOG(kTrace) << "[foo] trace ";
    LOG(kDebug) << "[foo] debug ";
    msleep(10);
    LOG(kInfo) << "[foo] info... ";
    LOG(kErr) << "[foo] error!!! ";
    LOG2(kErr, "%s, %d", "test log", 100);
    msleep(10);
    LOG_TAG(kErr, "BAR") << "[foo] error!!! ";
}

// 没有使用tag，显示会与foo不一样
void bar(int x)
{
    LOG(kTrace) << "[bar] trace ";
    LOG(kDebug) << "[bar] debug ";
    msleep(20);
    LOG(kInfo) << "[bar] info... ";
    msleep(10);
    LOG(kErr) << "[bar] error!!! ";
}

// 兼容原AMS_DEBUG、AMS_INFO、AMS_WARN和AMS_ERR
void ams()
{
    std::string tag = "ams";
    LogHelper::initLogHelper(tag);
    AMS_DEBUG("debug {} log", 111);
    AMS_INFO("info {}log", 222);
    AMS_WARN("warn {} log {}", 333, 11);
    AMS_ERR("err {} log", 444);
}

int main()
{
    //std::thread first(foo);     // spawn new thread that calls foo()
    //std::thread second(bar, 0);  // spawn new thread that calls bar(0)
    std::thread third(ams);  // spawn new thread that calls bar(0)

    std::cout << "execute concurrently...\n";

    // synchronize threads:
    //first.join();
    //second.join();
    third.join();

    std::cout << " completed.\n";
    return 0;
}
