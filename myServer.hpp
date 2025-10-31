#ifndef _SERVER_HPP_
#define _SERVER_HPP_

#include "epoll_mode.hpp"

class MyServer {
public:
    MyServer(int num);
    MyServer(const MyServer&) = delete;
    MyServer(MyServer&&) = delete;
    MyServer& operator=(const MyServer&) = delete;
    MyServer&& operator=(MyServer&&) = delete;

    void InitMyServer(int port, std::string);
    void StartMyServer();
private:
    void ReadMessage(int fd, std::string mess);
private:
    Server server_;
};

#endif // _SERVER_HPP_
