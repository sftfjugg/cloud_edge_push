#include <iostream>
#include <string>
#include <unistd.h>
#include <thread>
#include "shm_buf.h"
#include "messagebus.h"
#include "app_common.h"

const int kShmBufSize = 1024*1024*10;
const char *shm_name = "edge_client_cpp_py";

static bool s_isStart = false;

void startRecvSysInfo(int sleepSecond)
{
    if (s_isStart)
        return;
    std::thread([&]() {
        SharedMemoryBuffer shmbuf(shm_name, kShmBufSize);
        while (true)
        {
            if (!shmbuf.readable())
            {
                std::this_thread::sleep_for(std::chrono::seconds(sleepSecond));
                continue;
            }

            std::string read_data;
            shmbuf.read_shm(read_data);
            g_messagebus.sendMessage(kMsgSysInfo, read_data);
        }
    }).detach();
    s_isStart = true;
}
