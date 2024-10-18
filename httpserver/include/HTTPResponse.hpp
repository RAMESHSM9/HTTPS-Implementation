#pragma once

#include <string>
#include <unordered_map>
#include <functional>
#include "../../tcpserver/include/Clientstate.hpp"

namespace httpserver
{
    class HTTPResponse
    {
    public:
        HTTPResponse(tcpserver::Clientstate &iClientState, std::function<void(tcpserver::Clientstate &, std::string)> iResponseCallback);

        void setStatus(unsigned int statusCode, const std::string &iStatusMessage);

        void setHeader(const std::string &iKey, const std::string &iValue);

        void sendFile(const std::string &iFilePath);

        void send(const std::string &iResponsePayload);

        void send();

    private:
        tcpserver::Clientstate &_clientState;

        std::function<void(tcpserver::Clientstate &, std::string)> _responseCallback;

        std::unordered_map<std::string, std::string> _responseHeaders;

        unsigned int _statusCode;

        std::string _statusMessage;

        void sendResponse(const std::string &iBody = "");
    };
}
