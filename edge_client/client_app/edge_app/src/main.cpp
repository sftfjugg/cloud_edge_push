
#include <iostream>
#include <thread>
#include <limits>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include "tcpclient.h"
#include "app_common.h"
#include "appconfig_reader.h"
#include "messagehandler.h"
#include "loghelper.h"

using namespace RockLog;
using namespace boost;
using namespace std;
using boost::asio::ip::tcp;

int main(int argc, char *argv[])
{
    std::string tag = "client_app";
    LogHelper::initLogHelper(tag);
    try
    {
        // loadAppConfig
        LOG(kInfo) << "loading app config" << std::endl;
        const std::string cfgFilePath = boost::filesystem::current_path().string() + std::string("/client_app.cfg");
        ClientConfig_t cfg;
        if (boost::filesystem::exists(cfgFilePath))
            cfg = std::move(loadAppConfig(cfgFilePath));
        LOG(kInfo) << "cfg.serverIP: " << cfg.serverIP << ", cfg.serverPort: " << cfg.serverPort << std::endl;

        auto& msgHandler = MessageHandler::instance();
        std::thread t = msgHandler.start(cfg);

        t.join();
        msgHandler.stop();
    }
    catch (std::exception &e)
    {
        LOG(kErr) << "Exception: " << e.what() << std::endl;
    }

    return 0;
}


