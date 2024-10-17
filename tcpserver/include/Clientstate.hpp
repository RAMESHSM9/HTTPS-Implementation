#pragma once
#include <string>

namespace tcpserver
{

    struct Clientstate
    {

        int client_fd;

        std::string sendBuffer;

        std::string recvBuffer;
        Clientstate() : client_fd(-1) {}
        Clientstate(int iFD) : client_fd(iFD)
        {
        }
    };

} // namespace tcpserver
