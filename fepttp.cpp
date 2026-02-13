#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <mswsock.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <chrono>
#include <algorithm>
#include <unordered_map>

#pragma comment(lib, "ws2_32.lib")

class Fepttp{
public:
    Fepttp(int PORT){
        start_server(PORT);
    }

    ~Fepttp(){

    }

    int run(){
    while(true){

    }
    }

private:
    WSADATA wsa_data;

    int start_server(int PORT){
        std::cout << "Starting server on port " << PORT << std::endl;

        // Initialise WSA
        if(WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0){
            std::cout << "WSA Startup failed. Error: " << WSAGetLastError() << std::endl; WSACleanup(); return 1;
        }

        // Create the listening socket
        SOCKET lsocket = WSASocket(AF_INET, SOCK_STREAM, 0, nullptr, 0, WSA_FLAG_OVERLAPPED);
        if(lsocket == SOCKET_ERROR){
            std::cout << "Failed to create listening socket. Error" << WSAGetLastError() << std::endl; WSACleanup(); return 1;
        }

        // Bind lsocket to a port and listen on that port
        sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_port = htons(PORT);
        address.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");

        int bind_result = bind(lsocket, (sockaddr*) &address, sizeof(address));
        if(bind_result == SOCKET_ERROR){
            std::cout << "Failed to bind listening socket. Error: " << WSAGetLastError() << std::endl;
            WSACleanup(); return 1;
        }

        if(listen(lsocket, SOMAXCONN) == SOCKET_ERROR){
            std::cout << "Listen failed. Error: " << WSAGetLastError() << std::endl; WSACleanup(); return 1;
        }

        return 0;
    }
};

int main(){
    Fepttp server = Fepttp(8000);
    server.run();

    return 0;
}