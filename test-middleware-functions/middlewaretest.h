#include "../httpresponses.h"
#include "../json.hpp"
#include <iostream>

using json = nlohmann::json;

HTTPResponse Logger(HTTPRequest req, HTTPResponse res){
    std::string info = "Request info (logger):\n";
    info += (req.method + " " + req.target + " " + req.protocol + "\n");
    info += ("Content-Type: " + req.GetHeader("Content-Type") + "\n");
    info += ("Content-Length: " + req.GetHeader("Content-Length"));
    std::cout << info << std::endl;
    return res;
}

HTTPResponse JohnMiddleware(HTTPRequest req, HTTPResponse res){
    if(req.body.find("brazil nut") != std::string::npos){
        res.status_code = 403;
        json j;
        j["message"] = "Request rejected due to presence of brazil nuts";
        res.ApplicationJSON(j.dump());
        return res;
    }

    res.status_code = 200;
    json j;
    j["request target"] = req.target;
    j["subobject"]["Order of Alt(10)"] = 1814400;
    res.ApplicationJSON(j.dump());
    return res;
}