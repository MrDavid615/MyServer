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
    std::cout << mess << std::endl;

    Request req;
    if(req.Normalize(mess) == false) {
        ErrorMessageProcess(fd);
    }
    else if(req.GetMethod() == "GET") {
        GET_MethodProcess(fd, req);
    }
    else if(req.GetMethod() == "POST") {
        POST_MethodProcess(fd, req);
    }
    else {
        Undefine_MethodProcess(fd);
    }
}

void MyServer::InitMyServer(int port, std::string) {
    server_.Init(port);
}

void MyServer::StartMyServer() {
    server_.Run();
}

void MyServer::GET_MethodProcess(int fd, Request& req) {
    Response resp;
    if(req.GetURL() == "/") {
        resp.AddMessageHeaderEle("Content-Type", "text/html; charset=utf-8");
        std::string body = "<h1>Welcome to My Site</h1>";
        body += "<h1>GH I LOVE YOU !!!</h1>\n";
        resp.AddMessageHeaderEle("Content-Length", std::to_string(body.length()));
        resp.SetMessageBody(std::move(body));

        std::string buf = resp.Serialize();
        resp.PrintResponse();

        write(fd, buf.c_str(), buf.size());
        shutdown(fd, SHUT_WR);
    }
    else {
        resp.SetState(404);
        resp.SetReason("Not Found");
        resp.AddMessageHeaderEle("Content-Type", "text/html; charset=utf-8");

        std::string body = "<h1>Welcome to My Site</h1>";
        body += "<h1></h1>";
        body += "<h1>this page is making ... </h1>";
        resp.AddMessageHeaderEle("Content-Length", std::to_string(body.length()));
        resp.SetMessageBody(std::move(body));

        std::string buf = resp.Serialize();
        resp.PrintResponse();

        write(fd, buf.c_str(), buf.size());
        shutdown(fd, SHUT_WR);
    }
}

void MyServer::POST_MethodProcess(int fd, Request& req) {
    shutdown(fd, SHUT_WR);
}

void MyServer::Undefine_MethodProcess(int fd) {
    Response resp;
    resp.SetState(405);
    resp.SetReason("Method Not Allowed");
    resp.AddMessageHeaderEle("Content-Type", "text/html; charset=utf-8");
    resp.AddMessageHeaderEle("Allow", "GET, POST");

    std::string buf = resp.Serialize();
    resp.PrintResponse();

    write(fd, buf.c_str(), buf.size());
    shutdown(fd, SHUT_WR);
}

void MyServer::ErrorMessageProcess(int fd) {
    Response resp;
    resp.SetState(404);
    resp.SetReason("Not Found");
    resp.AddMessageHeaderEle("Content-Type", "text/html; charset=utf-8");

    std::string body = "<h1>Welcome to My Site</h1>";
    body += "<h1></h1>";
    body += "<h1>Message Format Error ... </h1>";
    resp.AddMessageHeaderEle("Content-Length", std::to_string(body.length()));
    resp.SetMessageBody(std::move(body));

    std::string buf = resp.Serialize();
    resp.PrintResponse();

    write(fd, buf.c_str(), buf.size());
    shutdown(fd, SHUT_WR);
}
