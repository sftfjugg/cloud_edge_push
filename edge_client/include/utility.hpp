#pragma once
#include <chrono>
#include <stdint.h>
#include <fstream>

#if defined(_WINDOWS)
#include <Winsock2.h>
#endif

namespace Utility
{
    static uint64_t getTimeStamp()
    {
        std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch());

        return ms.count();
    }

    template <class _X>
    struct AutoDeleterArry
    {
        AutoDeleterArry(_X *deletedObj) : obj_(deletedObj) {}

        AutoDeleterArry()
        {
            if (obj_)
                delete[] obj_;
            obj_ = 0;
        }

        _X *obj_;
    };

    static bool fileExist(const std::string &name)
    {
        std::ifstream f(name.c_str());
        return f.good();
    }

#if defined(__linux__)

#include <stdio.h>
#include <ifaddrs.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

    // https://blog.csdn.net/langeldep/article/details/8306603
    static std::string get_local_ip(std::string ifa_name = "")
    {
        static std::string ip;
        if (!ip.empty())
            return ip;
        char buf[64] = {0};
        struct ifaddrs *ifAddrStruct = NULL;
        void *tmpAddrPtr = NULL;

        getifaddrs(&ifAddrStruct);

        while (ifAddrStruct != NULL)
        {
            if (ifAddrStruct->ifa_addr->sa_family == AF_INET)
            { // check it is IP4
                // is a valid IP4 Address
                tmpAddrPtr = &((struct sockaddr_in *)ifAddrStruct->ifa_addr)->sin_addr;
                char addressBuffer[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);

                if (0 == strcmp(addressBuffer, "127.0.0.1"))
                {
                    ifAddrStruct = ifAddrStruct->ifa_next;
                    continue;
                }
                else
                {
                    if (ifa_name.empty() || (strcmp(ifa_name.c_str(), ifAddrStruct->ifa_name) == 0))
                    {
                        printf("%s IPV4 Address %s\n", ifAddrStruct->ifa_name, addressBuffer);
                        memcpy(buf, addressBuffer, INET_ADDRSTRLEN);
                        break;
                    }
                }
            }

            ifAddrStruct = ifAddrStruct->ifa_next;
        }
        ip = std::string(buf);
        return ip;
    }

#endif

#if defined(_WINDOWS)
    static std::string getLocalIpAddress()
    {
        WORD wVersionRequested = MAKEWORD(2, 2);

        WSADATA wsaData;
        if (WSAStartup(wVersionRequested, &wsaData) != 0)
            return "";

        char local[255] = {0};
        gethostname(local, sizeof(local));
        hostent *ph = gethostbyname(local);
        if (ph == NULL)
            return "";

        in_addr addr;
        memcpy(&addr, ph->h_addr_list[0], sizeof(in_addr)); // get the first ip address

        std::string localIP;
        localIP.assign(inet_ntoa(addr));

        WSACleanup();
        return localIP;
    }
#endif
}