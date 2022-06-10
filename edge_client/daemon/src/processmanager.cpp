#include <iostream>
#include "processmanager.h"
#include "processhelper.hpp"
#include "loghelper.h"

using namespace RockLog;

ProcessManager::ProcessManager(Config_t& cfg)
    : _cfg(cfg),
    _shmChker(cfg.sharedMemoryName, cfg.totalSharedMemorySize)
{
}

void ProcessManager::startCheckTimer()
{
    while (true)
    {
        checkProcessStatus();
        std::this_thread::sleep_for(std::chrono::milliseconds(_cfg.checkProcessInterval * 1000));
    }
}

bool ProcessManager::startProcess()
{
    {
        // 先删除子进程
        auto tup = ProcessHelper::getAllProcessPidsByName(_cfg.processName);
        auto pids = std::get<0>(tup);
        for (auto pid : pids)
            ProcessHelper::killProcess(pid);
    }
    {
        // 再删除非子进程
        auto tup = ProcessHelper::getAllProcessPidsByName(_cfg.processName);
        auto pids = std::get<0>(tup);
        for (auto pid : pids)
            ProcessHelper::killProcess(pid, false);
    }

    auto newtup = ProcessHelper::startProcess(_cfg.processName, "");

    _pid = std::get<0>(newtup);
    if (_pid <= 0)
    {
        LOG(kErr) << "start process error, pid: " << _pid << std::endl;
        return false;
    }
    return true;
}

void ProcessManager::checkProcessStatus()
{
    if (-1 == ProcessHelper::isRunning(_pid))
    {
        LOG(kErr) << "process " << _pid << " is not running! start process "
            << _cfg.processName << " again!" << std::endl;
        startProcess();
    }

#ifdef BOOST_POSIX_API
    else if (-2 == ProcessHelper::isRunning(_pid))
    {
        LOG(kErr) << "process " << _pid << " is zombie!" << std::endl;
        if (waitpid(_pid, NULL, 0) != _pid)
        {
            LOG(kErr) << "terminate zombie process " << _pid << " failed!" << std::endl;
        }
        startProcess();
    }
#endif

    if (_cfg.sharedMemoryCheck && -2 == _shmChker.check())
    {
        LOG(kErr) << "process " << _pid << " sharedmeory check failed! start process "
            << _cfg.processName << " again!" << std::endl;
        startProcess();
    }
}
