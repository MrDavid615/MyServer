#include "myServer.hpp"
#include "myHttp.hpp"

#include <iostream>
#include <functional>

MyServer::MyServer(int num)
    : server_(num)
{
    server_.SetPollerReadCallback(std::bind(&MyServer::ReadMessage, this, 
                                  std::placeholders::_1, std::placeholders::_2));
}

void MyServer::ReadMessage(int fd, std::string mess) {
    std::cout << "MyServer :: ReadMessage -> message in fd : " << fd << std::endl;
    // std::cout << mess << std::endl;

    Request req;
    if(req.Normalize(mess)) {
        // std::cout << req << std::endl;
    }

    Response resp;
    resp.AddAttribute("Content-Type", "text/html; charset=utf-8");
    std::string body = "<h1>Welcome to My Site</h1>";
    body += "<h1>GH I LOVE YOU !!!</h1>\n";
    resp.AddAttribute("Content-Length", std::to_string(body.length()));
    resp.SetMessageBodyMove(body);
    std::string buf = resp.Serialize();
    write(fd, buf.c_str(), buf.size());
    shutdown(fd, SHUT_WR);
}

void MyServer::InitMyServer(int port, std::string) {
    server_.Init(port);
}

void MyServer::StartMyServer() {
    server_.Run();
}
