#include <iostream>
#include <limits>
#include "appconfig_reader.h"
#include "processmanager.h"
#include "processhelper.hpp"
#include "loghelper.h"

using namespace RockLog;

int main(int argc, char *argv[])
{
    std::string tag = "daemon";
    LogHelper::initLogHelper(tag);
    try
    {
        // loadAppConfig
        const std::string cfgFilePath = boost::filesystem::current_path().string() + std::string("/daemonapp.cfg");
        Config_t cfg;
        if (boost::filesystem::exists(cfgFilePath))
            cfg = std::move(loadAppConfig(cfgFilePath));

        ProcessManager pm(cfg);
        pm.startProcess();
        pm.startCheckTimer();
    }
    catch (std::exception &e)
    {
        LOG(kErr) << "Exception: " << e.what() << std::endl;
    }

    return 0;
}
