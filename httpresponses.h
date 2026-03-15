#pragma once
#include "config.h"
#include "definitions.h"
#include "httpparsing.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <iostream>

using namespace Config;
using namespace Definitions;

class HTTPResponse {
private:
    std::vector<Header> headers;

    static inline std::unordered_map<int, std::string> status = {
        {100, "Continue"}, {101, "Switching Protocols"}, {102, "Processing"}, {103, "Early Hints"}, {200, "OK"}, {201, "Created"},
        {202, "Accepted"}, {203, "Non-Authoritative Information"}, {204, "No Content"}, {205, "Reset Content"},
        {206, "Partial Content"}, {207, "Multi-Status"}, {208, "Already Reported"}, {226, "IM Used"}, {300, "Multiple Choices"},
        {301, "Moved Permanently"}, {302, "Found"}, {303, "See Other"}, {304, "Not Modified"}, {307, "Temporary Redirect"},
        {308, "Permanent Redirect"}, {400, "Bad Request"}, {401, "Unauthorized"}, {402, "Payment Required"}, {403, "Forbidden"},
        {404, "Not Found"}, {405, "Method Not Allowed"}, {406, "Not Acceptable"}, {407, "Proxy Authentication Required"},
        {408, "Request Timeout"}, {409, "Conflict"}, {410, "Gone"}, {411, "Length Required"}, {412, "Precondition Failed"},
        {413, "Content Too Large"}, {414, "URI Too long"}, {415, "Unsupported Media Type"}, {416, "Range Not Satisfiable"},
        {417, "Expectation Failed"}, {418, "I'm a teapot"}, {421, "Misdirected Request"}, {422, "Unprocessable Content"},
        {423, "Locked"}, {424, "Failed Dependency"}, {425, "Too Early"}, {426, "Upgrade Required"}, {428, "Precondition Required"},
        {429, "Too Many Requests"}, {431, "Request Header Fields Too Large"}, {451, "Unavailable For Legal Reasons"},
        {500, "Internal Server Error"}, {501, "Not Implemented"}, {502, "Bad Gateway"}, {503, "Service Unavailable"},
        {504, "Gateway Timeout"}, {505, "HTTP Version Not Supported"}, {506, "Variant Also Negotiates"}, {507, "Insufficient Storage"},
        {508, "Loop Detected"}, {510, "Not Extended"}, {511, "Network Authentication Required"}
    };

    std::string FormattedDateAndTime(){
        auto now = std::chrono::system_clock::now();
        std::time_t _now = std::chrono::system_clock::to_time_t(now);
        std::tm * gmt = std::gmtime(&_now);
        char buf[32];
        if(std::strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT", gmt)){
            return buf;
        }
        return "";
    }

    std::string Assemble(){
        std::string res = "";

        // Assemble status line
        res += ("HTTP/1.1 " + std::to_string(status_code) + " " + status[status_code] + "\r\n");

        // Assemble response headers (controlled by server, not by user)
        res += "Server: Fepttp\r\n";
        res += ("Date: " + FormattedDateAndTime() + "\r\n");
        res += ("Cache-Control: " + cache_control + "\r\n");
        
        // Assemble representation headers

        // Assemble body

        // Return the assembled response
        return res;
    }

public:
    void SetHeader(std::string name, std::string value){
        // 
    }

    void GetHeader(std::string name){
        // This is application layer stuff
    }

    void AddHeader(std::string name, std::string value){
        Header h = Header(name, value);
        headers.push_back(h);
    };

    int status_code;

    // Default is "no-cache"
    std::string cache_control = "no-cache";

    void Send(){
        // Send the request to the server (call some server method; let the server handle the sending)
        // Temporary for testing:
        std::cout << Assemble() << std::endl;
    };
};