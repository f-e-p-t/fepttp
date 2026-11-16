#include "config_old.h"
#include <iostream>
#include <winsock2.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <chrono>
#include <algorithm>
#include <unordered_map>

using namespace Config;

// Request parsing
constexpr int COULD_NOT_FIND = -1;
constexpr int STOI_ERROR = -2;
constexpr int TOO_LARGE = -3;

struct SOCKET_AND_PROPS {
    SOCKET sock;
    std::time_t lastuse;
    char recv_buffer[RECV_BUFFER_LENGTH];
    std::string req_accumulator = "";
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
    idle_timeout.tv_sec = SELECT_TIMEOUT_S;
    idle_timeout.tv_usec = SELECT_TIMEOUT_MS;
    while(true){
        // Declare and clear fd_sets for readable and writeable sockets. Add the listening socket to readable
        fd_set open_sockets_read; fd_set open_sockets_write; FD_ZERO(&open_sockets_read); FD_ZERO(&open_sockets_write);
        FD_SET(listening_socket, &open_sockets_read);

        // Fill up both lists from conn_map, minus those that have timed out. Close the timed out sockets
        auto _current_time = std::chrono::system_clock::now();
        std::time_t current_time = std::chrono::system_clock::to_time_t(_current_time);
        std::vector<SOCKET> timed_out_sockets;
        for(const auto& entry : conn_map){
            if(socket_timed_out(entry.second.sock, current_time)){ timed_out_sockets.push_back(entry.second.sock); continue; }
            FD_SET(entry.second.sock, &open_sockets_read); FD_SET(entry.second.sock, &open_sockets_write);
        }
        
        for(const auto& sock : timed_out_sockets){ closesocket(sock); conn_map.erase(sock); }

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

            if(bytes == 0){ // i.e. TCP connection gracefully closed. Empty kernel buffer? Then you would get WSAEWOULDBLOCK
                std::cout << "Socket gracefully closed: " << sock << std::endl;
                closesocket(sock); conn_map.erase(sock); continue;
            }

            // Haven't 'continue'd yet - there is data in the buffer to be dealt with
            conn_map[sock].req_accumulator.append(recvbuf, bytes);
            set_socket_lastuse(sock);
            //std::cout << conn_map[sock].req_accumulator;

            int _header_end = header_end(conn_map[sock].req_accumulator);
            if(_header_end == COULD_NOT_FIND){ continue; }
            if(_header_end == TOO_LARGE){ closesocket(sock); conn_map.erase(sock); continue; }
            if(contains_TE(conn_map[sock].req_accumulator, _header_end)){ closesocket(sock); conn_map.erase(sock); continue; }

            int _body_length = body_length(conn_map[sock].req_accumulator, _header_end);
            if(_body_length == TOO_LARGE){ closesocket(sock); conn_map.erase(sock); continue; }
            if(_body_length == STOI_ERROR){ closesocket(sock); conn_map.erase(sock); continue; }
            if(_body_length == COULD_NOT_FIND){ _body_length = 0; }
            
            int _request_length = _header_end + 4 + _body_length;
            if(conn_map[sock].req_accumulator.length() < _request_length){ continue; }

            std::cout << conn_map[sock].req_accumulator.substr(0, _request_length) << std::endl; // Eventually - parse
            conn_map[sock].req_accumulator.erase(0, _request_length);

            // Accumulate until the accumulator has the complete request (headers + _body_length bytes of body). Then parse.
            // If a user undersends, idle timeout will close the socket eventually (yet to implement)

            // Finding the request ending:
            // Q: What if the CL is honest but simply too large?
            // A: Hardcap on the header and body size.
            // Q: What if a user injects an early CL: 1000000?
            // A: Unconditionally the headers end after \r\n\r\n. If 1000000 is more than the body length cap, close the socket.
            // Otherwise, take in 1000000 bytes as a request. This might cut into other requests on the socket, and the request
            // will be messed up. This will be caught by the full parser, which will be called after the request is fully collected.
            // The full parser will detect framing violations and close the socket immediately.
            // Q: What if the request contains "\r\n\r\n" before it contains CL?
            // A: Call the header ending finder first, then on success, call the CL finder (body length finder) and search before
            // the index of "\r\n\r\n". It won't find anything, and the request will end there.
            // Q: What about multiple requests in one buffer?
            // A: One you know the bounds of the request and you have it all appended to the string, parse it then cut it off the front
            // of the string, bringing the next request to the front. Keep appending the buffer to the string.

            // How to put this in code:
            // 
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

    void set_socket_lastuse(SOCKET sock){
        auto _current_time = std::chrono::system_clock::now();
        std::time_t current_time = std::chrono::system_clock::to_time_t(_current_time);
        conn_map[sock].lastuse = current_time;
    }

    bool socket_timed_out(SOCKET sock, std::time_t current_time){
        if(current_time >= conn_map[sock].lastuse + SOCKET_IDLE_TIMEOUT_S){ return true; }
        return false;
    }

    int header_end(std::string accumulator){
        if(accumulator.length() > REQ_HEADER_LENGTH_CAP){ return TOO_LARGE; }
        auto header_end_index = accumulator.find("\r\n\r\n");
        if(header_end_index == std::string::npos){ return COULD_NOT_FIND; }
        return header_end_index;
    }

    bool contains_TE(std::string accumulator, int header_end_index){
        if(header_end_index == std::string::npos){ return false; }
        auto TE_index = accumulator.find("Transfer-Encoding");
        if(TE_index != std::string::npos){ return true; }
        return false;
    }

    int body_length(std::string accumulator, int header_end_index){
        if(header_end_index == std::string::npos){ return COULD_NOT_FIND; }
        auto CL_index = accumulator.rfind("Content-Length", header_end_index);
        if(CL_index == std::string::npos){ return COULD_NOT_FIND; }

        auto CL_end_index = accumulator.find("\r\n", CL_index);
        if(CL_end_index == std::string::npos){ return COULD_NOT_FIND; }

        try{
            int bytes = std::stoi(accumulator.substr(CL_index + 16, CL_end_index)); // stoi may throw an errors
            if(bytes > REQ_BODY_LENGTH_CAP){ return TOO_LARGE; }
            return bytes;
        } catch(...){
            return STOI_ERROR;
        }
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