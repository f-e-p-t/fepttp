#include "../httpresponses.h"
#include <iostream>

HTTPResponse CreateResponse(HTTPRequest req, HTTPResponse res){
    res.status_code = 304;
    res.AddHeader("Header", "hhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhh");
    //res.AddHeader("This will break postman", "header text");
    return res; 
}