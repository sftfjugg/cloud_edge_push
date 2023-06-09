#include "appconfig_reader.h"
#include "app_common.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/algorithm/string.hpp>
#include <iostream>

using namespace boost;

static const char *kAppConfigRoot = "client";
static const char *kIsReconnect = "IsReconnect";
static const char *kReconnectInterval = "ReconnectInterval";
static const char *kHeartbeatInterval = "HeartbeatInterval";
static const char *kServerIP = "ServerIP";
static const char *kServerPort = "ServerPort";
static const char *kHttpServer = "HttpServer";
static const char *kClientType = "ClientType";


// load app configure
ClientConfig_t loadAppConfig(const std::string &filepath)
{
    property_tree::ptree pt;
    property_tree::ini_parser::read_ini(filepath, pt);
    property_tree::ptree client;
    client = pt.get_child(kAppConfigRoot);

    ClientConfig_t cfg;
    auto a = client.get_optional<bool>(kIsReconnect);
    if (a)
        cfg.isReconnect = *a;

    auto b = client.get_optional<int>(kReconnectInterval);
    if (b)
        cfg.reconnectInterval = *b;

    auto c = client.get_optional<int>(kHeartbeatInterval);
    if (c)
        cfg.heartbeatInterval = *c;

    auto d = client.get_optional<std::string>(kServerIP);
    if (d)
        cfg.serverIP = *d;

    auto e = client.get_optional<std::string>(kServerPort);
    if (e)
        cfg.serverPort = *e;

    auto f = client.get_optional<std::string>(kHttpServer);
    if (f)
        cfg.httpServer = *f;
    
    auto g = client.get_optional<std::string>(kClientType);
    if (g)
        cfg.clientType = *g;

    return cfg;
}
