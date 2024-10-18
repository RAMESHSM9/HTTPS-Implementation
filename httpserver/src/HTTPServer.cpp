#include <iostream>

#include "../include/HTTPServer.hpp"
#include "../../tcpserver/include/TCPServer.hpp"

namespace httpserver
{
    HTTPServer::HTTPServer() : _tcpServer(std::make_shared<tcpserver::TCPServer>())
    {
        //@ define the callbacks here
        if (_tcpServer)
        {
            //@ mechanism to facilitate tcp layer to communicate with http layer
            //@ on certain events like, when the request is recieved
            _tcpServer->setRecieveCallback([this](tcpserver::Clientstate &iClientState)
                                           { this->onRecv(iClientState); });
            _tcpServer->setListenCallback([this](int iServerFD)
                                          { this->onListen(iServerFD); });
        }
        else
        {
            //@ throw runtime exception
        }
    }

    HTTPServer::~HTTPServer() {}

    void HTTPServer::sendResponse(tcpserver::Clientstate &iClientState, const std::string &iResponse)
    {
        iClientState.bytesSent = 0;

        iClientState.sendBuffer = iResponse;

        _tcpServer->handleSend(iClientState.client_fd);
    }

    void HTTPServer::onRecv(tcpserver::Clientstate &iClientState)
    {
        std::cout << "recvd data headers are : " << std::endl;
        for (auto &itr : iClientState.headers)
        {
            std::cout << "header : " << itr.first << " value : " << itr.second << std::endl;
        }
        std::cout << "recvd data : path -> " << iClientState.httpResourcePath << std::endl;
        std::cout << "recvd data : http method -> " << iClientState.httpMethod << std::endl;

        HTTPRequest httpRequest(iClientState);

        HTTPResponse httpResponse(iClientState, [this](tcpserver::Clientstate &iClientState, const std::string &iResponse)
                                  { sendResponse(iClientState, iResponse); });

        if (_registerdURLRegistry.count(httpRequest.getPath()) > 0)
        {
            _registerdURLRegistry[httpRequest.getPath()](httpRequest, httpResponse);
        }
        else
        {
            httpResponse.setStatus(400, "Not Found");
            httpResponse.send();
        }
    }

    HTTPServer &HTTPServer::registerURL(const std::string &iResourcePath, std::function<void(HTTPRequest &, HTTPResponse &)> ihttpHandler)
    {
        _registerdURLRegistry[iResourcePath] = ihttpHandler;
        return *this;
    }

    void HTTPServer::onListen(int clientSocket)
    {
        std::cout << "came to the http server listen function" << std::endl;
        // calling the user given function once server is ready to start accepting the connections.
        onStartListening();
    }
    void HTTPServer::listen(int port, listenCallback onStart)
    {
        std::cout << "starting to listen  " << std::endl;

        setListenCallback(onStart);

        _tcpServer->initialize(port, "0.0.0.0");

        std::cout << "Server initialized." << std::endl;

        // Start the server
        _tcpServer->start();
    }

    void HTTPServer::setListenCallback(listenCallback listenCallback)
    {
        this->onStartListening = listenCallback;
    }
}