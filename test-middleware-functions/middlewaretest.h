#include "../httpresponses.h"
#include <iostream>

HTTPResponse CreateResponse(HTTPRequest req, HTTPResponse res){
    res.status_code = 304;
    return res; 
}