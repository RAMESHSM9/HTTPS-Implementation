#include "../include/TCPServer.hpp"
#include <cstring>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sstream>
#include "../include/Clientstate.hpp"

namespace tcpserver
{
    namespace
    {
        void setNonBlocking(int iFD)
        {
            int flags = fcntl(iFD, F_GETFL, 0);

            if (flags == -1)
            {
                std::string errorMessage = "Error getting socket flags!";
                throw std::runtime_error(errorMessage);
            }

            if (fcntl(iFD, F_SETFL, flags | O_NONBLOCK) < 0)
            {
                std::string errorMessage = "Could not set the fd " + std::to_string(iFD) + " to non-blocking state";
                throw std::runtime_error(errorMessage);
            }
        }

        void parseRequest(tcpserver::Clientstate &oClientState, std::string iRequestLine)
        {
            //@ stream uses space as default delimitier
            std::istringstream requestStream(iRequestLine);
            requestStream >> oClientState.httpMethod;
            requestStream >> oClientState.httpResourcePath;
            requestStream >> oClientState.httpVersion;
        }

        void parseHeaders(tcpserver::Clientstate &oClientState, std::string iHeaderData)
        {
            std::istringstream stream(iHeaderData);
            std::string line;
            size_t content_length = 0;

            // Parse each line of the headers
            while (std::getline(stream, line) && line != "\r")
            {
                if (line.find(": ") != std::string::npos)
                {
                    auto pos = line.find(": ");
                    std::string key = line.substr(0, pos);
                    std::string value = line.substr(pos + 2);
                    oClientState.headers[key] = value;

                    if (key == "Content-Length")
                    {
                        oClientState.contentLenghtOfMessage = std::stoul(value);
                    }
                }
            }
        }
    }

    TCPServer::TCPServer() : _serverFD(-1), _threadPool(5) /*thread pool size can also be determined by based on the hardware concurrency for better optimization*/
    {
    }

    TCPServer::~TCPServer()
    {
        if (_serverFD != -1)
        {
            close(_serverFD);
        }
    }

    void TCPServer::setRecieveCallback(std::function<void(Clientstate &iClientState)> iRecieveCallback)
    {
        _recieveCallback = iRecieveCallback;
    }

    void TCPServer::setListenCallback(std::function<void(int)> iListenCallback)
    {
        this->_listenCallback = iListenCallback;
    }

    void TCPServer::initialize(int iPort, std::string iIPAddress)
    {
        if ((_serverFD = socket(AF_INET, SOCK_STREAM, 0)) == 0)
        {
            std::string errorMessage = "Socket creation failed";
            throw std::runtime_error(errorMessage);
        }

        // Set socket options to reuse address and port
        int opt = 1;
        if ((setsockopt(_serverFD, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) < 0)
        {
            std::string errorMessage = "setsockopt - REUSE Address failed!";
            throw std::runtime_error(errorMessage);
        }

        std::cout << "Socket creation is done - File descriptor " << _serverFD << std::endl;

        struct sockaddr_in _serverAddress;
        _serverAddress.sin_family = AF_INET;
        _serverAddress.sin_addr.s_addr = INADDR_ANY;
        _serverAddress.sin_port = htons(iPort);
        socklen_t addrlen = sizeof(_serverAddress);

        if ((bind(_serverFD, (struct sockaddr *)&_serverAddress, addrlen)) < 0)
        {
            std::string errorMessage = "Binding Socket FD and adrress failed";
            throw std::runtime_error(errorMessage);
        }

        std::cout << "Socket Binding is done - File descriptor " << _serverFD << std::endl;

        if ((listen(_serverFD, 5)) < 0)
        {
            std::string errorMessage = "Listening over socket failed";
            throw std::runtime_error(errorMessage);
        }

        std::cout << "started listening over socket fd " << _serverFD << " Port " << iPort << std::endl;

        if ((_epollFD = epoll_create1(0)) < 0)
        {
            std::string errorMessage = "epoll creation failed";
            throw std::runtime_error(errorMessage);
        }

        std::cout << "epoll creation is done - File descriptor " << _epollFD << std::endl;

        _inboundOutboundThread = std::thread(&TCPServer::handleInboundOutbound, this);
        _inboundOutboundThread.detach();

        std::cout << "started the thread in a zombie mode to handle the epoll events, thread id" << _inboundOutboundThread.get_id() << std::endl;

        if (_listenCallback)
            _listenCallback(_serverFD);
    }

    void TCPServer::start()
    {

        struct sockaddr_in client_address;
        socklen_t addrlen = sizeof(client_address);
        int client_fd = -1;

        //@ has to keep accepting multiple connections
        while (true)
        {
            //@ accept - blocking API
            if ((client_fd = accept(_serverFD, (struct sockaddr *)&client_address, &addrlen)) < 0)
            {
                std::string errorMessage = "client connection failed";
                continue;
            }

            //@ make the client_fd as non-blocking - so that thread dont get blocked on rcv/send calls
            setNonBlocking(client_fd);

            std::cout << "Accepted a connection " << client_fd << std::endl;

            //@ add the client_fd to the intrest list of the epoll - so that we wake up only on notification
            struct epoll_event event;
            event.data.fd = client_fd;
            event.events = EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLET;
            if ((epoll_ctl(_epollFD, EPOLL_CTL_ADD, client_fd, &event)) < 0)
            {
                std::string errorMessage = "Adding client fd" + std::to_string(client_fd) + " to the epoll monitoring list failed";
                throw std::runtime_error(errorMessage);
            }

            //@ add the client_fd to clientState Datastructure
            _clientsState[client_fd] = Clientstate(client_fd);
        }
    }

    bool TCPServer::isRequestComplete(int iClientFD)
    {
        auto &clientState = _clientsState[iClientFD];

        if (!clientState.isCompleteHeadersRecieved)
        {
            //@ look for the end of the headers
            //@ take that data out of recieved buffer
            //@ then parse the headeders [Version, path, method, content-length...]

            //@ In HTTP Messagess,
            //@ 1. \r\n marks the EOL
            //@ 2. \r\n\r\n marks the end of the headers and the rest of the buffer is the payload/body
            size_t headerEndPos = clientState.recvBuffer.find("\r\n\r\n");

            if (headerEndPos != std::string::npos)
            {
                //@ end of the headers is found
                //@ slice the headers from the complete message
                std::string headers_data = clientState.recvBuffer.substr(0, headerEndPos + 4);
                clientState.recvBuffer.erase(0, headerEndPos + 4);

                //@ first line in the HTTP message is the METHOD RESOURCEPATH VERSION
                size_t endOfRequestLine = headers_data.find("\r\n");
                std::string requestLine = headers_data.substr(0, endOfRequestLine);
                parseRequest(clientState, requestLine);

                std::string headers = headers_data.substr(endOfRequestLine + 2);
                parseHeaders(clientState, headers);

                clientState.isCompleteHeadersRecieved = true;
            }
        }
        else if (clientState.isCompleteHeadersRecieved && clientState.contentLenghtOfMessage > 0)
        {
            if (clientState.recvBuffer.size() >= clientState.contentLenghtOfMessage)
            {
                // Full body received, process the complete request
                clientState.payload_data = clientState.recvBuffer.substr(0, clientState.contentLenghtOfMessage);
                clientState.recvBuffer.erase(0, clientState.contentLenghtOfMessage); // Remove the body from the buffer

                // Call the callback with the complete data (headers + body)
                clientState.isCompleteRequestRecieved = true;
                return true;
            }
        }
        else if (clientState.isCompleteHeadersRecieved && clientState.contentLenghtOfMessage == 0)
        {
            //@ No body (i.e., only headers), process the request now
            //@ for GET DELETE methods...
            clientState.isCompleteRequestRecieved = true;
            return true;
        }

        return false;
    }

    void TCPServer::handleRecv(int iClientFd)
    {
        std::cout << "TCPServer::handleRecv, Message from Client fd" << iClientFd << std::endl;

        auto &clientState = _clientsState[iClientFd];

        char readBuffer[1024];

        int bytesRead = recv(iClientFd, readBuffer, 1024, 0);

        // clientState.recvBuffer = readBuffer;

        std::cout << "Number of bytes read " << bytesRead << std::endl;

        if (bytesRead > 0)
        {
            //@ message
            clientState.recvBuffer.append(readBuffer, bytesRead);

            if (isRequestComplete(clientState.client_fd))
            {
                //@ call the HTTP layer to process the request
                _recieveCallback(clientState);
            }
        }
        else if (bytesRead == 0)
        {
            //@ received on when client closes the connection
            //@ perform the clean
            std::cout << "Client disconnected, FD" << clientState.client_fd << std::endl;

            close(clientState.client_fd);

            _clientsState.erase(clientState.client_fd);
        }
        else
        {
            //@ recieved when there is drop of the message or some kernel error
            //! we try again to read the kernel buffer (as the message would be already be in the kernel buffer)
            if (errno != EWOULDBLOCK && errno != EAGAIN)
            {
                std::cerr << "Error in recv: " << strerror(errno) << std::endl;

                handleError(clientState.client_fd);
            }
            else
            {
                //@ more data to be received
                return;
            }
        }
    }

    void TCPServer::handleSend(int iClientFd)
    {
        auto &clientState = _clientsState[iClientFd];

        if (clientState.bytesSent < clientState.sendBuffer.size())
        {
            size_t bytesToBeSent = clientState.sendBuffer.size() - clientState.bytesSent;

            size_t bytesSent = send(iClientFd, clientState.sendBuffer.c_str() + clientState.bytesSent, bytesToBeSent, 0);

            if (bytesToBeSent > 0)
            {
                clientState.bytesSent += bytesSent;

                if (clientState.bytesSent == clientState.sendBuffer.size())
                {
                    // If all data has been sent, reset the state
                    std::cout << "All data sent to client." << std::endl;

                    // making http as stateless protocol. remove from the states map
                    clientState.clear();
                }
            }
            else
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    // wait for the next opportunity to send
                    return;
                }
                else
                {
                    // An error occurred, handle it
                    std::cerr << "Error in send." << std::endl;
                    handleError(iClientFd);
                }
            }
        }
    }

    void TCPServer::handleError(int iClientFd)
    {
        // Clean up state on error
        auto &clientState = _clientsState[iClientFd];

        std::cerr << "Socket error, closing connection for client FD" << clientState.client_fd << std::endl;

        close(clientState.client_fd);

        _clientsState.erase(clientState.client_fd);
    }

    void TCPServer::handleInboundOutbound()
    {
        //@ keep an infinite loop  - to look for the notifications constantly
        struct epoll_event evlist[10]; // max 10 event notifs in interest list
        while (true)
        {
            int numberOfFdsInReadyList = epoll_wait(_epollFD, evlist, 10, -1);

            for (int i = 0; i < numberOfFdsInReadyList; i++)
            {
                int fd = evlist[i].data.fd;

                //! for each of the type of notification (Read, Write, Error)
                //! have a seprate thread handle each of the cases
                if (evlist[i].events & EPOLLIN)
                {
                    //@ handle recv
                    _threadPool.push([this, fd](int thread_id)
                                     { this->handleRecv(fd); });
                }
                else if (evlist[i].events & EPOLLOUT)
                {
                    //@ handle send
                    _threadPool.push([this, fd](int thread_id)
                                     { this->handleSend(fd); });
                }
                else if (evlist[i].events & EPOLLERR)
                {
                    //@ handle error
                    _threadPool.push([this, fd](int thread_id)
                                     { this->handleError(fd); });
                }
            }
        }
    }

}