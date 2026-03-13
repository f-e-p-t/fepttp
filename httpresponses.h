#pragma once
#include "config.h"
#include "definitions.h"
#include "httpparsing.h"
#include <vector>
#include <string>
#include <iostream>

using namespace Config;
using namespace Definitions;

class HTTPResponse {
private:
    std::vector<Header> headers;

    void Assemble(){
        
    }

public:
    void SetHeader(std::string name, std::string value){
        Header h = Header(name, value);
        headers.push_back(h);
    };

    int status_code;

    void Send(){
        // Send the request to the server (call some server method; let the server handle the sending)
    };
};