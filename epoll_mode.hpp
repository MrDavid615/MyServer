#ifndef _EPOLL_MODE_HPP_
#define _EPOLL_MODE_HPP_

#include <iostream>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <functional>

/**
 * Acceptor is used to liten socket
 * the new Connect fd returned by accept will be distribe to Poller
 */
class Acceptor {
public:
    Acceptor()
        : fd_(socket(AF_INET, SOCK_STREAM, 0))
    {   
        if(fd_ < 0) {
            std::cout << "Acceptor :: Init Error" << std::endl;
            exit(-1);
        }
    }

    ~Acceptor() {
        if(fd_ >= 0) close(fd_);
    }

    void Bind(int port, std::string ip = "") {
        int opt = 1;
        setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));   // Port Reuse In Wait Time

        addr_.sin_port = htons(port);
        addr_.sin_addr.s_addr = INADDR_ANY;
        addr_.sin_family = AF_INET;
        if(bind(fd_, (const sockaddr*)&addr_, sizeof(addr_)) < 0) {
            std::cout << "Acceptor :: Bind Error" << std::endl;
            exit(-1);
        }
    }

    void Listen() {
        if(listen(fd_, SOMAXCONN) < 0) {
            std::cout << "Acceptor :: Listen Error" << std::endl;
            exit(-1);
        }
    }

    int Accept() {
        sockaddr_in addr;
        int len = sizeof(addr);
        int fd = accept(fd_, (sockaddr*)&addr, (socklen_t*)&len);
        if(fd < 0) {
            std::cout << "Acceptor :: Accept Fail" << std::endl;
            return -1;
        }
        setNonBlock(fd);
        return fd;
    }

    static void setNonBlock(int fd) {
        int fl = fcntl(fd, F_GETFL, 0);
        fcntl(fd, F_SETFL, fl | O_NONBLOCK);
    }

private:
    int fd_;
    sockaddr_in addr_;
};

/**
 * SubReactor based on epoll
 * wakeFd_ is fd interface, Acceptor write 8 bytes data to wakeFd_ to wake up Poller
 */
class Poller {
public:
    Poller() 
        : epollFd_(epoll_create1(EPOLL_CLOEXEC))
        , wakeFd_(eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC))
        , connNum_(0)
        , conn_cb_([](int fd)-> void {})
        , read_cb_([](int fd, std::string mess) -> void {})
        , disConn_cb_([](int fd)-> void {})
    {
        if(epollFd_ < 0 || wakeFd_ < 0) {
            std::cout << " Poller :: Poller() Error" << std::endl;
            exit(-1);
        }

        epoll_event ev;
        ev.data.fd = wakeFd_;
        ev.events = EPOLLIN;
        epoll_ctl(epollFd_, EPOLL_CTL_ADD, wakeFd_, &ev);
    }

    ~Poller() {
        if(epollFd_ >= 0) close(epollFd_);
        if(wakeFd_ >= 0) close(wakeFd_);
    }

    Poller(const Poller&) = delete;
    Poller& operator=(const Poller&) = delete;
    Poller(Poller&&) = default;

    void AddFd(int fd, uint32_t evClass) {
        std::unique_lock<std::mutex> lock(mut_);
        newFdVec.emplace_back(fd, evClass);
    }

    void RemoveFd(int fd) {
        epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, nullptr);
        close(fd);
    }

    void Run() {
        std::vector<epoll_event> readyEv(128);
        while(true) {
            int readyCount = epoll_wait(epollFd_, (epoll_event*)&readyEv[0], readyEv.size(), -1);
            if(readyCount < 0) {
                std::cout << "Poller :: Run Error by EPOLL_WAIT" << std::endl;
                exit(-1);
            }

            for(int i = 0; i < readyCount; i++) {
                int fd = readyEv[i].data.fd;
                if(fd == wakeFd_) {
                    uint64_t buf;
                    read(wakeFd_, &buf, sizeof(uint64_t));
                    
                    std::vector<std::pair<int, uint32_t>> tmp_vec; {
                        std::unique_lock<std::mutex> lock(mut_);
                        tmp_vec.swap(newFdVec);
                    }

                    for(auto& p : tmp_vec) {
                        epoll_event ev;
                        ev.data.fd = p.first;
                        ev.events = p.second;
                        epoll_ctl(epollFd_, EPOLL_CTL_ADD, p.first, &ev);
                        conn_cb_(p.first);
                        connNum_.fetch_add(1, std::memory_order_relaxed);
                        std::cout << "Poller :: fd " << ev.data.fd << " is added success" << std::endl;
                    }

                    newFdVec.clear();
                }
                else {   // 普通客户端 fd
                    char buf[1024];
                    ssize_t n;
                    std::string data;
                    data.reserve(4096);
                
                    while ((n = read(fd, buf, sizeof(buf))) > 0) {
                        data.append(buf, n);          // 按实际读到的字节数拼
                    }
                
                    if (n == 0) {                     // 对端关闭
                        std::cout << "Poller :: client in fd : " << fd << " close" << std::endl;
                        disConn_cb_(fd);
                        RemoveFd(fd);
                        connNum_.fetch_sub(1, std::memory_order_relaxed);
                    } 
                    else if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
                        std::cout << "Poller :: client error in fd : " << fd;
                        std::cout << " errno is " << strerror(errno) << std::endl;
                        disConn_cb_(fd);
                        RemoveFd(fd);
                        connNum_.fetch_sub(1, std::memory_order_relaxed);
                    } 
                    else { // n < 0 && errno == EAGAIN：缓冲区读空，正常结束
                        // std::cout << std::endl << std::endl << data << std::endl;
                        read_cb_(fd, data);
                    }

                }
            }
        }
    }

    int GetFd() const {
        return wakeFd_;
    }

    int GetConnNum() const {
        return connNum_.load(std::memory_order_relaxed);
    }

    void SetConnectCallback(std::function<void(int)> cb) {
        conn_cb_ = cb;
    }

    void SetReadCallback(std::function<void(int, std::string)> cb) {
        read_cb_ = cb;
    }

    void SetDisConnectCallback(std::function<void(int)> cb) {
        disConn_cb_ = cb;
    }

private:
    int epollFd_;
    int wakeFd_;
    std::atomic<int> connNum_;
    std::mutex mut_;
    std::vector<std::pair<int, uint32_t>> newFdVec;
    std::function<void(int)> conn_cb_;
    std::function<void(int, std::string)> read_cb_;
    std::function<void(int)> disConn_cb_;
};

/**
 * One loop Per thread
 * all thread has a Poller
 */
class ThreadPool {
public:
    ThreadPool(int num)
        : vec_(num)
    {
        if(num <= 0) {
            std::cout << "thread num must > 1" << std::endl;
            exit(-1);
        }
    }

    Poller& GetPoller() {
        int minConn = vec_[0].GetConnNum();
        int indx = 0;
        for(int i = 1; i < vec_.size(); i++) {
            if(minConn > vec_[i].GetConnNum()) {
                minConn = vec_[i].GetConnNum();
                indx = i;
            }
        }
        return vec_[indx];
    }

    void Run() {
        for(int i = 0; i < vec_.size(); i++) {
            std::thread th(&ThreadPool::func, this, i);
            std::cout << "ThreadPool :: the " << i << "th thread is running" << std::endl;
            th.detach();
        }
    }

    void SetConnCallback(std::function<void(int)> cb) {
        for(auto& poller : vec_) {
            poller.SetConnectCallback(cb);
        }
    }

    void SetReadCallback(std::function<void(int, std::string)> cb) {
        for(auto& poller : vec_) {
            poller.SetReadCallback(cb);
        }
    }

    void SetDisConnCallback(std::function<void(int)> cb) {
        for(auto& poller : vec_) {
            poller.SetDisConnectCallback(cb);
        }
    }

private:
    std::vector<Poller> vec_;
    void func(int i) {
        vec_[i].Run();
    }
};

/**
 * The code sample

#include "epoll_mode.hpp"

int main() {
    int threadNum = 3; 
    Server server(threadNum);

    server.SetPollerConnCallback(myFunc1);
    server.SetPollerReadCallback(myFunc2);
    server.SetPollerDisConnCallback(myFunc3);

    int port = 8080;
    server.Init(port);
    server.Run();
    return 0;
} 

 */
class Server {
public:
    Server(int num) 
        : threadNum_(num)
        , acceptor_()
        , pool_(threadNum_)
        , conn_cb_([](int fd)-> void {})
        , read_cb_([](int fd, std::string mess) -> void {})
        , disConn_cb_([](int fd)-> void {})
    {}

    void Init(int port, std::string ip = "") {
        acceptor_.Bind(port, ip);
        pool_.SetConnCallback(conn_cb_);
        pool_.SetReadCallback(read_cb_);
        pool_.SetDisConnCallback(disConn_cb_);
    }

    void Run() {
        acceptor_.Listen();
        pool_.Run();

        while(true) {
            int fd = acceptor_.Accept();

            if(fd < 0) {
                std::cout << "Server :: Accept() ret < 0" << std::endl;
                continue;
            }
            else {
                std::cout << "Server :: new Connect" << std::endl;
            }

            auto& poller = pool_.GetPoller();
            poller.AddFd(fd, EPOLLIN | EPOLLERR);
            int wakeFd = poller.GetFd();
            uint64_t buf = 1;
            write(wakeFd, &buf, sizeof(buf));
        }
    }

    void SetPollerConnCallback(std::function<void(int)> cb) {
        conn_cb_ = cb;
    }

    void SetPollerReadCallback(std::function<void(int, std::string)> cb) {
        read_cb_ = cb;
    }

    void SetPollerDisConnCallback(std::function<void(int)> cb) {
        disConn_cb_ = cb;
    }

private:
    int threadNum_;
    Acceptor acceptor_;
    ThreadPool pool_;
    std::function<void(int)> conn_cb_;
    std::function<void(int, std::string)> read_cb_;
    std::function<void(int)> disConn_cb_;
};

#endif // _EPOLL_MODE_HPP_
