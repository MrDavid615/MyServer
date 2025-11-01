#ifndef _MYHTTP_H_
#define _MYHTTP_H_

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <unistd.h>

class HttpMessage {
public:
    HttpMessage()
        : ver("HTTP/1.1")
    {}

    const std::string& GetVersion() const {
        return ver;
    }

    const std::string& GetMessageBody() const {
        return body;
    }

    std::string GetMessageHeadersEle(const std::string& key){
        if(headers.find(key) == headers.end()) {
            return "";
        }
        return headers[key];
    }

    void ClearMessageHeaders() {
        headers.clear();
    }

    void SetVersion(std::string&& version) {
        ver = version;
    }

    void SetMessageBody(std::string&& mess) {
        body = std::move(mess);
    }

    void AddMessageHeaderEle(std::string key, std::string val) {
        headers.emplace(key, val);
    }

protected:
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

    std::string ver;
    std::unordered_map<std::string, std::string> headers;
    std::string body;
};

class Request : public HttpMessage {
public:
    const std::string& GetMethod() const {
        return method;
    }

    const std::string& GetURL() const {
        return url;
    }

    void SetURL(const std::string& _url) {
        url = _url;
    }

    void SetMethod(const std::string& _method) {
        method = _method;
    }

    bool Normalize(const std::string& message) {
        // remove_r(message);
        ClearMessageHeaders();

        size_t beg = 0;
        size_t end = 0;
        end = message.find(' ', beg);
        if(end == std::string::npos) return false;
        method = message.substr(beg, end - beg);

        beg = message.find_first_not_of(' ', end);
        if(beg == std::string::npos) return false;
        end = message.find(' ', beg);
        if(end == std::string::npos) return false;
        url = message.substr(beg, end - beg);

        beg = message.find_first_not_of(' ', end);
        if(beg == std::string::npos) return false;
        if((end = message.find_first_of("\r\n", beg)) == std::string::npos) return false;
        ver = message.substr(beg, end - beg);

        while(end < message.size()) {
            if(message[end] == '\n') end = end + 1;
            else beg = end + 2;
            if(message[beg] == '\n' || message[beg] == '\r') break;
            if( (end = message.find(':', beg)) == std::string::npos) return false;
            std::string key = message.substr(beg, end - beg);
            
            beg = end + 1;  
            beg = message.find_first_not_of(' ', beg);
            if(beg == std::string::npos) return false;
            if((end = message.find_first_of("\r\n", beg)) == std::string::npos) return false;
            std::string val = message.substr(beg, end - beg);

            headers.emplace(key, val);
        }

        if(message[beg] == '\n') beg++;  // \n
        else beg += 2;
        if(beg < message.size()) body = message.substr(beg);
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

    void PrintRequest() {
        std::cout << "HTTP Request : " << std::endl;
        std::cout << "Method : " << method << std::endl;
        std::cout << "URL : " << url << std::endl;
        std::cout << "Version : " << ver << std::endl;
        std::cout << std::endl;

        std::cout << "Request Head : " << std::endl;
        for(auto& p : headers) {
            std::cout << p.first << " : " << p.second << std::endl;
        }
        std::cout << std::endl;
        std::cout << "Request Body : " << std::endl;
        std::cout << body << std::endl;
    }

private:
    std::string method;         // GET POST PUT ...
    std::string url;            // URL
};

class Response : public HttpMessage {
public:
    Response()
        : state(200)
        , reason("OK")
    {}

    void SetState(int val = 200) {
        if(val < 100 || val > 600) {
            std::cout << "http state code is error!" << std::endl;
            return;
        }
        state = val;
    }

    void SetReason(std::string&& str = "OK") {
        reason = str;
    }

    int GetState() const {
        return state;
    }

    const std::string& GetReason() {
        return reason;
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

    bool Normalize(std::string& message) {
        // remove_r(message);
        ClearMessageHeaders();

        size_t beg = 0;
        size_t end = 0;

        if( (end = message.find(' ', beg)) == std::string::npos) return false;
        ver = message.substr(beg, end - beg);

        beg = message.find_first_not_of(' ', end);
        if(beg == std::string::npos) return false;
        if( (end = message.find(' ', beg)) == std::string::npos) return false;
        state = std::stoi(message.substr(beg, end - beg));

        beg = message.find_first_not_of(' ', end);
        if(beg == std::string::npos) return false;
        if( (end = message.find_first_of("\n\r", beg)) == std::string::npos) return false;
        reason = message.substr(beg, end - beg);

        while(end < message.size()) {
            if(message[beg] == '\n') beg = end + 1;
            else beg = end + 2;
            if(message[beg] == '\n' || message[beg] == '\r') break;
            if( (end = message.find(':', beg)) == std::string::npos) return false;
            std::string key = message.substr(beg, end - beg);
            
            beg = end + 1;  
            beg = message.find_first_not_of(' ', beg);
            if(beg == std::string::npos) return false;
            if((end = message.find_first_of("\r\n", beg)) == std::string::npos) return false;
            std::string val = message.substr(beg, end - beg);

            headers.emplace(key, val);
        }

        if(message[beg] == '\n') beg++;
        else beg += 2;
        if(beg < message.size()) body = message.substr(beg);

        return true;
    }

    void PrintResponse() {
        std::cout << "HTTP Response : " << std::endl;
        std::cout << "Version : " << ver << std::endl;
        std::cout << "State Code : " << state << std::endl;
        std::cout << "State : " << reason << std::endl; 
        std::cout << std::endl;

        std::cout << "Response Head : " << std::endl;
        for(auto& p : headers) {
            std::cout << p.first << " : " << p.second << std::endl;
        }
        std::cout << std::endl;

        std::cout << "Response Body : " << std::endl;
        std::cout << body << std::endl;
    }

private:
    int state;
    std::string reason;
};

#endif // _MYHTTP_H_