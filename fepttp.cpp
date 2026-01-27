#include <iostream>
#include <winsock2.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <chrono>
#include <algorithm>
#include <unordered_map>

#define IDLE_TIMEOUT_S 5
#define IDLE_TIMEOUT_ADDITIONAL_MS 0
#define RECV_BUFFER_LENGTH 64
#define SEND_BUFFER_LENGTH 64

#define TRANSFER_ENCODING_CHUNKED -1
#define NOT_TRA_ENC_CHU_CON_LEN -2

// MAKE A VARIABLE LENGTH BUFFER (STRING) THAT IS FILLED UP WITH FULL HTTP REQS BY THE SMALL BUFFER.
// GIVE IT A SIZE CAP
// MAKE SURE THIS WHOLE PROCESS IF COMPLETE BEFORE PROCEEDING TO THE NEXT SOCKET IN THE READABLE SOCKETS LOOPS
struct SOCKET_AND_PROPS {
    SOCKET sock;
    std::time_t lastuse_time;
    char recv_buffer[RECV_BUFFER_LENGTH];
    std::string request_buffer = "";
    char send_buffer[SEND_BUFFER_LENGTH];
};

class Fepttp{
public:
    Fepttp(int PORT){
        startServer(PORT);
    }

    ~Fepttp(){
        closeServer();
    }

    int run(){
    idle_timeout.tv_sec = IDLE_TIMEOUT_S;
    idle_timeout.tv_usec = IDLE_TIMEOUT_ADDITIONAL_MS;
    while(true){
        // Declare and clear fd_sets for readable and writeable sockets. Add the listening socket to readable
        fd_set open_sockets_read; fd_set open_sockets_write; FD_ZERO(&open_sockets_read); FD_ZERO(&open_sockets_write);
        FD_SET(listening_socket, &open_sockets_read);

        // Fill up both lists from conn_map
        for(const auto& entry : conn_map){
            FD_SET(entry.second.sock, &open_sockets_read); FD_SET(entry.second.sock, &open_sockets_write);
        }

        // Filter the socket lists and shift the readable resp. writeable ones to the front. Adjust length accordingly
        if(select(0, &open_sockets_read, &open_sockets_write, nullptr, &idle_timeout) == SOCKET_ERROR){
            std::cout << "Error checking sockets for readability and writeability. Error: " << WSAGetLastError() << std::endl;
            return 1;
        }

        // Handle all of the readable sockets
        for(int i = 0; i < open_sockets_read.fd_count; i++){
            const SOCKET sock = open_sockets_read.fd_array[i];

            if(sock == listening_socket){ // Special case: the listening socket
                SOCKET new_socket = accept(listening_socket, nullptr, nullptr);

                if(new_socket == INVALID_SOCKET){ // Handle different error types
                    auto error = WSAGetLastError();
                    if(error == WSAEWOULDBLOCK){ std::cout << "Empty accept queue" << std::endl; }
                    else{ std::cout << "Error accepting from the accept queue. Error: " << error << std::endl; }

                } else{ // Add the new socket to conn_map
                    auto _current_time = std::chrono::system_clock::now();
                    std::time_t current_time = std::chrono::system_clock::to_time_t(_current_time);
                    conn_map.insert({new_socket, { new_socket, current_time, {}, "", {} }});
                }
                continue;
            }

            // All other readable sockets
            int bytes = recv(sock, conn_map[sock].recv_buffer, recv_buffer_length, 0);
            auto recvbuf = conn_map[sock].recv_buffer;

            if(bytes == SOCKET_ERROR){
                auto error = WSAGetLastError();
                if(error != WSAEWOULDBLOCK){
                    std::cout << "Error receiving data from socket " << sock << ". Error: " << error <<
                    ". Closing socket" << std::endl;
                    closesocket(sock); conn_map.erase(sock);
                }
                continue;
            }

            if(bytes == 0){ // i.e. TCP connection terminated. Empty kernel buffer? Then you would get WSAEWOULDBLOCK
                closesocket(sock); conn_map.erase(sock);
                std::cout << "Closed socket: " << sock << std::endl;
                continue;
            }

            // Haven't broken yet - there is data in the buffer to be dealt with
            conn_map[sock].request_buffer.append(recvbuf, bytes);
            int _request_length = request_length(conn_map[sock].request_buffer);
            std::cout << conn_map[sock].request_buffer;
            std::cout << _request_length << std::endl;

            set_socket_lastuse_conn_map(sock);

            // Time to write a function that returns instructions on how to find the end of HTTP req
        }
    }
    }
private:
    SOCKET listening_socket;
    WSADATA _wsa;
    u_long imode = 1; // Non-blocking mode
    sockaddr_in address;
    int bind_result;
    timeval idle_timeout;
    int recv_buffer_length = RECV_BUFFER_LENGTH;

    void set_socket_lastuse_conn_map(SOCKET sock){
        auto _current_time = std::chrono::system_clock::now();
        std::time_t current_time = std::chrono::system_clock::to_time_t(_current_time);
        conn_map[sock].lastuse_time = current_time;
    }

    int request_length(std::string request){
        auto index = request.find("Transfer-Encoding: chunked");
        if(index != std::string::npos){ return TRANSFER_ENCODING_CHUNKED; }
        index = request.find("Content-Length");
        if(index != std::string::npos){
            auto content_length_end = request.find("\r\n\r\n", index + 16);
            auto content_length_length = content_length_end - index - 16;
            return index + 16 + content_length_length + 4 + std::stoi(request.substr(index + 16, content_length_length));
            //auto req_length = request.find()
        }
        return NOT_TRA_ENC_CHU_CON_LEN;
    }

    // Contains all current open TCP connections (sockets), and certain properties of them
    std::unordered_map<SOCKET, SOCKET_AND_PROPS> conn_map;

    // Create the listening socket
    int startServer(int PORT){
        std::cout << "Starting server on port " << PORT << std::endl;

        // Initialise WSA
        if(WSAStartup(MAKEWORD(2, 2), &_wsa) != 0){
            std::cout << "WSA Startup failed. Error: " << WSAGetLastError() << std::endl;
            std::exit(1);
        }
        
        // Create socket to be listened on
        listening_socket = socket(AF_INET, SOCK_STREAM, 0);
        if(listening_socket == INVALID_SOCKET){
            std::cout << "Socket creation failed. Error: " << WSAGetLastError() << std::endl;
            std::exit(1);
        }

        // Set listening socket to non-blocking mode
        if(ioctlsocket(listening_socket, FIONBIO, &imode) != 0){
            std::cout << "ioctlsocket failed. Error: " << WSAGetLastError() << std::endl;
            std::exit(1);
        }

        // Set address
        address.sin_family = AF_INET;
        address.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
        address.sin_port = htons(PORT);

        // Bind listening socket to a port
        bind_result = bind(listening_socket, (SOCKADDR*) &address, sizeof(address));
        if(bind_result == SOCKET_ERROR){
            std::cout << "Bind listening socket failed. Error: " << WSAGetLastError() << std::endl;
            bind_result = closesocket(listening_socket);
            if(bind_result == SOCKET_ERROR){
                std::cout << "Close listening socket failed. Error: " << WSAGetLastError() << std::endl;
            }
            std::exit(1);
        }

        if(listen(listening_socket, SOMAXCONN) == SOCKET_ERROR){ // SOMAXCONN is the maximum backlog (connection queue) size
            std::cout << "Listen failed. Error: " << WSAGetLastError();
            std::exit(1);
        }

        return 0;
    }

    int closeServer(){
        closesocket(listening_socket); WSACleanup();
        return 0;
    }
};

int main(){
    Fepttp server = Fepttp(8000);

    server.run();

    return 0;
}