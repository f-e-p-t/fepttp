#include "../http-requests-responses/httpresponses.h"
#include "../nlohmann-json-header/json.hpp"
#include <iostream>

using json = nlohmann::json;

HTTPReqRes Logger(HTTPRequest req, HTTPResponse res){
    std::string info = "Request info (logger):\n";
    info += (req.method + " " + req.target + " " + req.protocol + "\n");
    info += ("Content-Type: " + req.GetHeader("Content-Type") + "\n");
    info += ("Content-Length: " + req.GetHeader("Content-Length"));
    std::cout << info << std::endl;
    return HTTPReqRes(req, res);
}

HTTPReqRes AddHeaderToReq(HTTPRequest req, HTTPResponse res){
    Header h = Header("New-Header", "new header text");
    req.AddHeader(h);
    req.SetHeader("New-Header", "different new header text");
    return HTTPReqRes(req, res);
}

HTTPReqRes JaneMiddleware(HTTPRequest req, HTTPResponse res){
    res.AddHeader("Jane-Middleware", "this header was added by Jane Middleware");
    return HTTPReqRes(req, res);
}

HTTPReqRes JohnMiddleware(HTTPRequest req, HTTPResponse res){
    if(req.body.find("brazil nut") != std::string::npos){
        res.status_code = 403;
        json j;
        j["message"] = "Request rejected due to presence of brazil nuts in body";
        res.ApplicationJSON(j.dump());
        return HTTPReqRes(req, res);
    }

    res.status_code = 200;
    json j;
    j["your request target"] = req.target;
    j["subobject"]["example number"] = 1814400;
    res.ApplicationJSON(j.dump());
    return HTTPReqRes(req, res);
}