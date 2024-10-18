#pragma once
#include <string>
#include <netinet/in.h>
#include <unordered_map>
#include "ctpl_stl.h"
#include "Clientstate.hpp"

namespace tcpserver
{
    class TCPServer
    {
    private:
        int _serverFD;

        int _epollFD;

        std::unordered_map<int, Clientstate> _clientsState;

        ctpl::thread_pool _threadPool;

        std::thread _inboundOutboundThread;

        std::function<void(Clientstate &iClientState)> _recieveCallback;

        std::function<void(int)> _listenCallback;

        bool isRequestComplete(int iClientFD);

    public:
        TCPServer();

        void initialize(int iPort, std::string iIPAddress);

        void start();

        void setRecieveCallback(std::function<void(Clientstate &iClientState)> iRecieveCallback);

        void setListenCallback(std::function<void(int)> iListenCallback);

        void handleInboundOutbound();

        void handleRecv(int iClientFd);

        void handleSend(int iClientFd);

        void handleError(int iClientFd);

        ~TCPServer();
    };
}