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
            fd_set open_sockets_read;
            fd_set open_sockets_write;
            FD_ZERO(&open_sockets_read);
            FD_ZERO(&open_sockets_write);
            FD_SET(listening_socket, &open_sockets_read);

            // Fill up both lists from conn_map
            for(const auto& entry : conn_map){
                FD_SET(entry.second.sock, &open_sockets_read);
                FD_SET(entry.second.sock, &open_sockets_write);
            }

            // Filter the socket lists and shift the readable resp. writeable ones to the front. Adjust length accordingly
            if(select(0, &open_sockets_read, &open_sockets_write, nullptr, &idle_timeout) == SOCKET_ERROR){
                std::cout << "Error checking sockets for readability and writeability. Error: " << WSAGetLastError() << std::endl;
                return 1;
            }

            for(int i = 0; i < open_sockets_read.fd_count; i++){ // Handle all of the readable sockets
                const SOCKET sock = open_sockets_read.fd_array[i];
                //std::cout << "Socket ready for reading at index " << i << ": " << open_sockets_read.fd_array[i] << std::endl;

                if(sock == listening_socket){ // The listening socket
                    SOCKET new_socket = accept(listening_socket, nullptr, nullptr);

                    if(new_socket == INVALID_SOCKET){ // Handle different error types
                        auto error = WSAGetLastError();
                        if(error == WSAEWOULDBLOCK){
                            std::cout << "Empty accept queue" << std::endl;
                        } else{
                            std::cout << "Error accepting from the accept queue. Error: " << error << std::endl;
                            continue;
                        }

                    } else{ // Add the new socket to conn_map
                        //std::cout << "Opened new socket: " << new_socket << std::endl;
                        auto _current_time = std::chrono::system_clock::now();
                        std::time_t current_time = std::chrono::system_clock::to_time_t(_current_time);
                        conn_map.insert({new_socket, {
                            new_socket, current_time, {}, {}
                        }});
                    }

                } else{ // All other readable sockets
                    bool request_in_progress = true;
                    bool flag = false;
                    conn_map[sock].request_buffer = "";
                    while(request_in_progress){
                        int bytes_received = recv(sock, conn_map[sock].recv_buffer, recv_buffer_length, 0);
                        if(bytes_received == SOCKET_ERROR){
                            auto error = WSAGetLastError();
                            if(error == WSAEWOULDBLOCK){ flag = true; break; }
                            else{
                                std::cout << "Error receiving data from socket " << sock << ". Error: " << error <<
                                ". Closing socket" << std::endl;
                                closesocket(sock); conn_map.erase(sock); flag = true; break;
                            }

                        } else if(bytes_received == 0){ // i.e. TCP connection terminated
                            closesocket(sock); conn_map.erase(sock); std::cout << "Closed socket: " << sock << std::endl;
                            flag = true; break;

                        } else{ // If data was successfully received
                            conn_map[sock].request_buffer.append(conn_map[sock].recv_buffer, bytes_received);
                            //std::cout << conn_map[sock].recv_buffer;
                            //std::cout.write(conn_map[sock].recv_buffer, bytes_received);
                            //std::cout << conn_map[sock].request_buffer;
                            set_socket_lastuse_conn_map(sock);

                            // Time to write a function that returns instructions on how to find the end of HTTP req
                        }
                    }
                    if(flag){ continue; }

                    //int bytes_received = recv(sock, conn_map[sock].recv_buffer, recv_buffer_length, 0);
                    //
                    //// Handle different error types
                    //if(bytes_received == SOCKET_ERROR){
                    //    auto error = WSAGetLastError();
                    //    if(error == WSAEWOULDBLOCK){
                    //        continue;
                    //    } else{
                    //        std::cout << "Error receiving data from socket " << sock << ". Error: " <<
                    //        error << ". Closing socket" << std::endl;
                    //        closesocket(sock);
                    //        conn_map.erase(sock);
                    //        continue;
                    //    }
                    //
                    //// i.e. TCP connection terminated
                    //} else if(bytes_received == 0){
                    //    closesocket(sock);
                    //    conn_map.erase(sock);
                    //    std::cout << "Closed socket: " << sock << std::endl;
                    //
                    //// If data was successfully received
                    //} else{
                    //    std::cout.write(conn_map[sock].recv_buffer, bytes_received);
                    //    set_socket_lastuse_conn_map(sock);
                    //}
                }
            }

            // Handle writeable sockets
            // MAKE IT SO WRITE IS ONLY CALLED AFTER RECV HAS CONSUMED THE WHOLE REQUEST
        }
    }
private:
    SOCKET listening_socket;
    WSADATA _wsa;
    u_long imode = 1; // Non-blocking mode
    sockaddr_in address;
    int bind_result;
    timeval idle_timeout;
    char recv_buffer[RECV_BUFFER_LENGTH];
    int recv_buffer_length = RECV_BUFFER_LENGTH;

    void set_socket_lastuse_conn_map(SOCKET sock){
        auto _current_time = std::chrono::system_clock::now();
        std::time_t current_time = std::chrono::system_clock::to_time_t(_current_time);
        conn_map[sock].lastuse_time = current_time;
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