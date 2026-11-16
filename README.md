# FEPTTP

## Overview
FEPTTP is a lightweight single-header HTTP server for Windows built with C++. The server uses **IO Completion Ports (IOCP)** for an asynchronous, non-blocking architecture with high throughput. It uses **HTTP1.1** only and requires **C++11** or later.

## Requests and Reponses
FEPTTP packages raw HTTP requests and responses into objects with methods to freely manipulate them.

## Function Chain
- Custom request-handler (middleware) functions must take two parameters - an HTTPRequest object and an HTTPResponse object.
- After processing the two objects, the functions must return the request and response as a wrapped pair: HTTPReqRes(request, response).
- Push the functions sequentially into the 'server.chain' vector. The server executes them in that order when a request arrives.

## Example Code
Example code of a very simple web backend can be found in /example-usage.