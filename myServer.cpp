#include "myServer.hpp"

#include <iostream>
#include <functional>

MyServer::MyServer(int num)
    : server_(num)
{
    server_.SetPollerReadCallback(std::bind(&MyServer::ReadMessage, this, 
                                  std::placeholders::_1, std::placeholders::_2));
}

void MyServer::ReadMessage(int fd, std::string mess) {
    std::cout <<std::endl << "message in fd : " << fd << std::endl;
    std::cout << mess << std::endl;
}

void MyServer::InitMyServer(int port, std::string) {
    server_.Init(port);
}

void MyServer::StartMyServer() {
    server_.Run();
}
