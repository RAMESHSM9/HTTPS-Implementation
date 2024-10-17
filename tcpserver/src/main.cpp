#include <iostream>
#include <memory>
#include <thread>
#include "../include/TCPServer.hpp"

using namespace std;

int main()
{
    std::unique_ptr<tcpserver::TCPServer> tcpServ = std::make_unique<tcpserver::TCPServer>();

    if (tcpServ)
    {
        tcpServ->initialize(8080, "0.0.0.0");

        tcpServ->start();
    }
}