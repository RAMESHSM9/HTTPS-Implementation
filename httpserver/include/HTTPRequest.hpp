#pragma once

#include <string>
#include <unordered_map>
#include "../../tcpserver/include/Clientstate.hpp"

namespace httpserver
{
    class HTTPRequest
    {
    public:
        HTTPRequest(tcpserver::Clientstate &state);

        std::string getMethod() const;

        std::string getPath() const;

        std::string getHeader(const std::string &key) const;

        std::string getBody() const;

    private:
        tcpserver::Clientstate &_clientState;
    };
}
