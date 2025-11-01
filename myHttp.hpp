#ifndef _MYHTTP_H_
#define _MYHTTP_H_

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

class Request {
public:
    bool Normalize(std::string& message) {
        remove_r(message);

        int beg = 0;
        int end = 0;
        end = message.find(' ', beg);
        method = message.substr(beg, end - beg);

        beg = end + 1;
        end = message.find(' ', beg);
        url = message.substr(beg, end - beg);

        beg = end + 1;
        end = message.find('\n', beg);
        ver = message.substr(beg, end - beg);

        while(end < message.size() && message[end] == '\n') {
            beg = end + 1;
            if(message[beg] == '\n') {
                if(method == "GET") {
                    return true;
                }
                else {
                    break;
                }
            }
            end = message.find(':', beg);
            std::string key = message.substr(beg, end - beg);
            
            beg = end + 1;  
            if(message[beg] == ' ') beg++; // skip the ' ' after ':'
            end = message.find('\n', beg);
            std::string val = message.substr(beg, end - beg);

            headers.emplace(key, val);
        }
        beg++;  // \n
        body = message.substr(beg);
        return true;
    }

    std::string Serialize() {
        std::string req;
        // request line
        req = method + " " + url + " " + ver + "\r\n";
        // request head
        for(auto &p : headers) {
            req += p.first + ": " + p.second + "\r\n";
        }
        req += "\r\n";
        // request body
        req += body;

        return req;
    }

private:
    void remove_r(std::string& mess) {
        int s = 0;
        int q = 0;
        while(q < mess.size()) {
            if(mess[q] == '\r') {
                q++;
            }
            else {
                mess[s] = mess[q];
                q++;
                s++;
            }
        }
        mess.resize(s);
    }

public:
    std::string method;         // GET POST PUT ...
    std::string url;            // URL
    std::string ver;            // "HTTP/1.1"
    std::unordered_map<std::string, std::string> headers;
    std::string body;
};

class Response {
public:
    Response()
        : ver("HTTP/1.1")
        , state(200)
        , reason("OK")
    {}

    void SetVersion(std::string = "HTTP/1.1") {
        ver = "HTTP/1.1";
    }

    void SetState(int val = 200) {
        if(val < 100 || val > 600) {
            std::cout << "http state code is error!" << std::endl;
            return;
        }
        state = val;
    }

    void SetReason(std::string str = "OK") {
        reason = str;
    }

    void AddAttribute(std::string key, std::string val) {
        if(headers.find(key) == headers.end()) {
            headers.emplace(key, val);
        }
    }

    void SetMessageBodyCopy(const std::string& str) {
        body = str;
    }

    void SetMessageBodyMove(const std::string& str) {
        body = std::move(str);
    }

    std::string Serialize() {
        std::string resp;
        // request line
        resp = ver + " " + std::to_string(state) + " " + reason + "\r\n";
        // request head
        for(auto &p : headers) {
            resp += p.first + ": " + p.second + "\r\n";
        }
        resp += "\r\n";
        // request body
        resp += body;

        return resp;
    }

public:
    std::string ver;
    int state;
    std::string reason;
    std::unordered_map<std::string, std::string> headers;
    std::string body;
};

std::ostream& operator<<(std::ostream& _cout, Request& req) {
    _cout << "HTTP Request : " << std::endl;
    _cout << "Method : " << req.method << std::endl;
    _cout << "URL : " << req.url << std::endl;
    _cout << "Version : " << req.ver << std::endl;
    _cout << std::endl;

    _cout << "Request Head : " << std::endl;
    for(auto& p : req.headers) {
        _cout << p.first << " : " << p.second << std::endl;
    }
    _cout << std::endl;
    _cout << "Request Body : " << std::endl;
    _cout << req.body << std::endl;

    return _cout;
}

std::ostream& operator<<(std::ostream& _cout, Response& resp) {
    _cout << "HTTP Response : " << std::endl;
    _cout << "Version : " << resp.ver << std::endl;
    _cout << "State Code : " << resp.state << std::endl;
    _cout << "State : " << resp.reason << std::endl; 
    _cout << std::endl;

    _cout << "Response Head : " << std::endl;
    for(auto& p : resp.headers) {
        _cout << p.first << " : " << p.second << std::endl;
    }
    _cout << std::endl;

    _cout << "Response Body : " << std::endl;
    _cout << resp.body << std::endl;

    return _cout;
}


#endif // _MYHTTP_H_