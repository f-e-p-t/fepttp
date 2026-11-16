# FEPTTP

## Overview
FEPTTP is a lightweight single-header HTTP server for Windows built with C++. The server uses IO Completion Ports (IOCP) for an asynchronous, non-blocking architecture with high throughput.

## Requirements and Advice
bruh.

## Creating Backends
FEPTTP packages raw HTTP requests and responses into objects. The objects have methods used to manipulate incoming requests and build outgoing responses. For every incoming HTTP request, the request and response are created as a pair (class HTTPReqRes). Response-building functions (middleware) must take two inputs (request and response) and return a pair. The pair is built with: return HTTPReqRes(request, response).
After creating a server object, put the functions in order into the server.chain vector. The server will execute them in sequence when a request arrives. Examples are in this repository at example-usage/.
