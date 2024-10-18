#include "../include/HTTPRequest.hpp"

namespace httpserver
{
    HTTPRequest::HTTPRequest(tcpserver::Clientstate &iClientState) : _clientState(iClientState) {}

    std::string HTTPRequest::getMethod() const
    {
        return _clientState.httpMethod;
    }

    std::string HTTPRequest::getPath() const
    {
        return _clientState.httpResourcePath;
    }
    std::string HTTPRequest::getHeader(const std::string &key) const
    {
        if (_clientState.headers.find(key) != _clientState.headers.end())
        {
            return _clientState.headers.at(key);
        }
        return "";
    }
    std::string HTTPRequest::getBody() const
    {
        return _clientState.payload_data;
    }
}