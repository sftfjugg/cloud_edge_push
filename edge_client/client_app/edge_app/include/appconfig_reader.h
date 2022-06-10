#pragma once

#include <string>
#include "app_common.h"


/*
客户端配置文件读取，一般在程序初始化时候加载一次
*/
ClientConfig_t loadAppConfig(const std::string &filepath);
