#include "../include/TCPServer.hpp"
#include <cstring>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <fcntl.h>
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
    }

    void TCPServer::start()
    {
        _inboundOutboundThread = std::thread(&TCPServer::handleInboundOutbound, this);

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

    void TCPServer::handleRecv(int iClientFd)
    {
        std::cout << "TCPServer::handleRecv fd" << iClientFd << std::endl;

        auto &clientState = _clientsState[iClientFd];

        char readBuffer[1024];

        int bytesRead = recv(iClientFd, readBuffer, 1024, 0);

        clientState.recvBuffer = readBuffer;

        std::cout << "Number of bytes read " << bytesRead << std::endl;

        std::cout << "Message from Client " << iClientFd << " is " << clientState.recvBuffer << std::endl;
    }

    void TCPServer::handleSend(int iClientFd)
    {
        //  sending data
        std::string st;
        std::getline(std::cin, st);
        send(iClientFd, st.c_str(), st.size(), 0);
    }

    void TCPServer::handleError(int iClientFd)
    {
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