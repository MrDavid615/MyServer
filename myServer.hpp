#ifndef _SERVER_HPP_
#define _SERVER_HPP_

#include "epoll_mode.hpp"
#include "myHttp.hpp"

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
    void GET_MethodProcess(int fd, Request& req);
    void POST_MethodProcess(int fd, Request& req);
    void Undefine_MethodProcess(int fd);
    void ErrorMessageProcess(int fd);
private:
    Server server_;
};

#endif // _SERVER_HPP_
