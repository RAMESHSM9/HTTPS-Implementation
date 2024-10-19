#pragma once
#include <string>
#include <unordered_map>

namespace tcpserver
{

    struct Clientstate
    {

        int client_fd;

        std::string sendBuffer;

        std::string recvBuffer;

        bool isCompleteRequestRecieved;

        bool isCompleteHeadersRecieved;

        std::string httpMethod;

        std::string httpVersion;

        std::string httpResourcePath;

        std::unordered_map<std::string, std::string> headers;

        std::string payload_data;

        int contentLenghtOfMessage;

        int bytesSent;

        Clientstate() : client_fd(-1), isCompleteRequestRecieved(false), isCompleteHeadersRecieved(false), contentLenghtOfMessage(0), bytesSent(0)
        {
        }

        Clientstate(int iFD) : client_fd(iFD), isCompleteRequestRecieved(false), isCompleteHeadersRecieved(false), contentLenghtOfMessage(0), bytesSent(0)
        {
        }

        void clear()
        {
            sendBuffer.clear();

            recvBuffer.clear();

            isCompleteRequestRecieved = false;

            isCompleteHeadersRecieved = false;

            httpMethod.clear();

            httpVersion.clear();

            httpResourcePath.clear();

            headers.clear();

            payload_data.clear();

            contentLenghtOfMessage = 0;

            bytesSent = 0;
        }
    };

} // namespace tcpserver
