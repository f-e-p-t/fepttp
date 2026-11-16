# FEPTTP

## Overview
FEPTTP is a lightweight single-header HTTP server for Windows built with C++. The server uses IO Completion Ports (IOCP) for an asynchronous, non-blocking architecture with high throughput.

## Requirements and Advice
This server needs C++11 or later and only uses HTTP1.1.
This project already contains the nlohmann json header. Beware of including it twice!
Delete the example-usage folder if necessary.

## Creating Backends
Server settings including buffer sizes, idle timeout period, and displaying info in the terminal can be changed in configuration/config.h.
FEPTTP packages raw HTTP requests and responses into objects. The objects have methods used to manipulate incoming requests and build outgoing responses. Response-building (middleware) functions must take two inputs - request and response - and return a pair. The pair is built with: HTTPReqRes(request, response).
After creating a server object, put the functions in order into the server.chain vector. The server will execute them in sequence when a request arrives. Examples are in this repository at example-usage/.