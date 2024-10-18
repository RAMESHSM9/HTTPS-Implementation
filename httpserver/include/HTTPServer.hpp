#pragma once

#include <memory>
#include "../include/HTTPRequest.hpp"
#include "../include/HTTPResponse.hpp"
#include "../../tcpserver/include/Clientstate.hpp"

namespace tcpserver
{
    class TCPServer;
}
namespace httpserver
{

    class HTTPServer
    {
    private:
        std::shared_ptr<tcpserver::TCPServer> _tcpServer;

        void handleRequest();

        void sendResponse(tcpserver::Clientstate &iClientState, const std::string &iResponse);

        void onListen(int iServerFD);

        void onRecv(tcpserver::Clientstate &iClientState);

        void onError(tcpserver::Clientstate &iClientState);

        std::unordered_map<std::string, std::function<void(HTTPRequest &, HTTPResponse &)>> _registerdURLRegistry;

        using listenCallback = std::function<void()>;
        listenCallback onStartListening;

    public:
        HTTPServer();

        ~HTTPServer();

        void setListenCallback(listenCallback onListenCallback);

        HTTPServer &registerURL(const std::string &iResourcePath, std::function<void(HTTPRequest &, HTTPResponse &)> ihttpHandler);

        void listen(int port, listenCallback onStart);
    };
}