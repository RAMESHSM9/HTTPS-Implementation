#include "../include/HTTPResponse.hpp"
#include <sstream>
#include <fstream>

namespace httpserver
{
    namespace
    {
        std::string getContentType(const std::string &filePath)
        {
            static std::unordered_map<std::string, std::string> mimeTypes = {
                {".html", "text/html"},
                {".css", "text/css"},
                {".js", "application/javascript"},
                {".png", "image/png"},
                {".jpg", "image/jpeg"},
                {".gif", "image/gif"},
                // Add more mime types as needed
            };

            std::size_t dotPos = filePath.find_last_of('.');
            if (dotPos != std::string::npos)
            {
                std::string ext = filePath.substr(dotPos);
                if (mimeTypes.find(ext) != mimeTypes.end())
                {
                    return mimeTypes[ext];
                }
            }

            // Default content type
            return "text/plain";
        }
    }
    HTTPResponse::HTTPResponse(tcpserver::Clientstate &iClientState, std::function<void(tcpserver::Clientstate &, std::string)> iResponseCallback) : _clientState(iClientState), _responseCallback(iResponseCallback), _statusCode(200), _statusMessage("OK")
    {
    }

    void HTTPResponse::setHeader(const std::string &iKey, const std::string &iValue)
    {
        _responseHeaders[iKey] = iValue;
    }

    void HTTPResponse::setStatus(unsigned int iStatusCode, const std::string &iStatusMessage)
    {
        _statusCode = iStatusCode;
        _statusMessage = iStatusMessage;
    }

    void HTTPResponse::sendResponse(const std::string &iBody)
    {
        std::ostringstream responseStream;

        // Build the status line
        responseStream << "HTTP/1.1 " << _statusCode << " " << _statusMessage << "\r\n";

        // Add the headers stored in the map
        for (const auto &header : _responseHeaders)
        {
            responseStream << header.first << ": " << header.second << "\r\n";
        }

        // Add the Content-Length header if the body is not empty
        if (!iBody.empty())
        {
            responseStream << "Content-Length: " << iBody.size() << "\r\n";
        }

        // End headers section
        responseStream << "\r\n";

        // Add the response body if it's not empty
        if (!iBody.empty())
        {
            responseStream << iBody;
        }

        // Convert the response to a string
        std::string responseStr = responseStream.str();

        _responseCallback(_clientState, responseStr);
    }

    void HTTPResponse::send(const std::string &iBody)
    {
        sendResponse(iBody);
    }

    void HTTPResponse::send()
    {
        sendResponse();
    }

    // Send the contents of a file as the HTTP response
    void HTTPResponse::sendFile(const std::string &filePath)
    {
        std::ifstream file(filePath);
        if (!file.is_open())
        {
            // Send a 404 error if the file is not found
            setHeader("Content-Type", "text/plain");
            sendResponse("404 Not Found: Unable to load the requested file.");
            return;
        }

        // Read the file content into a string
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string fileContent = buffer.str();

        // Set the content type based on the file extension
        setHeader("Content-Type", getContentType(filePath));

        sendResponse(fileContent);
    }
}