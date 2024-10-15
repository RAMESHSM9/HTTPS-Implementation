#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <unistd.h>
#include <cstring>
#include <thread>
#include <sys/epoll.h>
#include "../include/ctpl_stl.h"

using namespace std;

/*
Steps
1. Create socket
2. Create address struct
3. Bind the scoket and address struct
4. listen
5. start accepting the connections
*/

void handleClient(int client_fd)
{
    while (true)
    {
        //@ recieve the message
        char buffer[1024] = {0};
        recv(client_fd, buffer, sizeof(buffer), 0);
        cout << "Message from client: " << buffer << endl;

        string st;
        std::getline(std::cin, st);

        int n = st.length();

        // declaring character array (+1 for null
        // character)
        char message[n + 1];

        // copying the contents of the string to
        // char array
        strcpy(message, st.c_str());
        // sending data
        send(client_fd, message, strlen(message), 0);
    }
    close(client_fd);
}

int main()
{
    int epoll_fd = epoll_create1(0); // 0 indiactes deafult to epoll_create() - older verion
    //@ create socket using socket() sys call
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0); // ip4 address TCP/IP protocol

    //@ create the sockaddress for our socket
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;         // IPV4 address
    serverAddress.sin_port = htons(8080);       // port of our server would be 8080
    serverAddress.sin_addr.s_addr = INADDR_ANY; // listening on any available IP on the machine

    //@ Bind the socket and the address - using bind()
    bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress));

    //@ listen - backlog connections - 5
    listen(serverSocket, 5);
    cout << "Ready to accept connections" << endl;

    ctpl::thread_pool pool(5);

    epoll_event serverEvent = {}, events[10]; // events max 10 event notifications
    serverEvent.data.fd = serverSocket;
    serverEvent.events = EPOLLIN;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, serverSocket, &serverEvent) < 0)
    {
        cout << "Adding server fd to epoll failed" << endl;
        close(serverSocket);
        return;
    }

    std::thread epoll_thred([&]()
                            {
        
        while(true)
        {
            int numberOfEvents = epoll_wait(epoll_fd,events,10, -1);
            for(int i=0;i<numberOfEvents;i++)
            {
                //@ there is new connection request
                if(events[i].data.fd == serverSocket)
                {
                    int clientSocket = accept(serverSocket, nullptr, nullptr);
                    if (clientSocket != -1)
                    {
                        epoll_event clientEvent = {};
                        clientEvent.data.fd = clientSocket;
                        clientEvent.events = EPOLLIN | EPOLLET;

                        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, serverSocket, &serverEvent) < 0)
                        {
                            cout << "Adding client fd to epoll failed" << endl;
                            close(clientSocket);
                        }
                    }
                }
                else
                {
                    //@ there is read event on one of the client fd
                    int client_fd = events[i].data.fd;
                    if(events[i].events & EPOLLIN)
                    {
                        pool.push([client_fd](int id){
                                handleClient(client_fd);
                        });
                    }
                }
            }
        } });

    epoll_thred.join();
    close(serverSocket);
    close(epoll_fd);
}