#include "../../httpserver/include/HTTPServer.hpp"

#include <iostream>

using namespace std;

int main()
{
    // Create the TCP server using the factory function
    const int PORT = 8000;
    httpserver::HTTPServer *server = new httpserver::HTTPServer();

    if (server)
    {
        // Initialize the server with a specific port and (optional) IP address
        // server->initialize(8080);  // Default is localhost if no IP is provided

        std::cout << "Server initialized." << std::endl;

        // Start the server
        server->registerURL("/users", [](httpserver::HTTPRequest &req, httpserver::HTTPResponse &res)
                            {
                     cout << "send all users from thsi route" << endl;

                        res.setHeader("header1", "value1");
                        res.setStatus(200, "ok done");
                        res.send("this is the bosy of http response"); })
            .registerURL("/shops", [](httpserver::HTTPRequest &req, httpserver::HTTPResponse &res)
                         {
                     cout << "send all the shops from thsi route" << endl;
                     res.send("all shops"); })
            .registerURL("/index.html", [](httpserver::HTTPRequest &req, httpserver::HTTPResponse &res)
                         {
                     cout << "send the index.html file" << endl;
                     res.sendFile("./applications" + req.getPath()); })
            .registerURL("/tictactoe.html", [](httpserver::HTTPRequest &req, httpserver::HTTPResponse &res)
                         {
                     cout << "send the index.html file" << endl;
                     res.sendFile("./applications" + req.getPath()); })
            .registerURL("/users", [](httpserver::HTTPRequest &req, httpserver::HTTPResponse &res)
                         {
                     cout << "send all the shops from thsi route" << endl;
                     res.send("all shops"); })
            .registerURL("/addshop", [](httpserver::HTTPRequest &req, httpserver::HTTPResponse &res)
                         { res.send("shop added ssuccessfully"); })
            .listen(PORT, [&]()
                    { cout << "server started listening on port : " << PORT << endl; });
    }
    else
    {
        std::cout << "Failed to create http server instance." << std::endl;
    }

    return 0;
}