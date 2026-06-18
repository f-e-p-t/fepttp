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
    res.status_code = 200;
    json JohnJSON;
    JohnJSON["Target of request"] = req.target;
    JohnJSON["subobject"]["dsuga"] = "tsat";
    res.ApplicationJSON(JohnJSON.dump());
    return res; 
}