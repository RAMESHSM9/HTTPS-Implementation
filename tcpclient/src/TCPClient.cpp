#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <unistd.h>
#include <cstring>
using namespace std;

int main()
{
    // creating socket
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);

    // specifying address
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    // sending connection request
    connect(clientSocket, (struct sockaddr *)&serverAddress,
            sizeof(serverAddress));

    while (true)
    {
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
        send(clientSocket, message, strlen(message), 0);

        //@ recieve the message
        char buffer[1024] = {0};
        recv(clientSocket, buffer, sizeof(buffer), 0);
        cout << "Message from server: " << buffer << endl;
    }

    // closing socket
    close(clientSocket);

    return 0;
}