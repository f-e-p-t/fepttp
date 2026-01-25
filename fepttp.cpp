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

struct SOCKET_AND_PROPS {
    SOCKET sock;
    std::time_t lastuse_time;
};

class Fepttp{
public:
    Fepttp(int PORT){
        startServer(PORT);
    }

    ~Fepttp(){
        closeServer();
    }

    void run(){
        idle_timeout.tv_sec = IDLE_TIMEOUT_S;
        idle_timeout.tv_usec = IDLE_TIMEOUT_ADDITIONAL_MS;

        while(true){
            // Declare and clear lists of readable and writeable sockets. Add the listening socket to readable
            fd_set open_sockets_read;
            fd_set open_sockets_write;
            FD_ZERO(&open_sockets_read);
            FD_ZERO(&open_sockets_write);
            FD_SET(listening_socket, &open_sockets_read);

            // Fill up both lists from connections
            for(const auto& entry : connections_map){
                FD_SET(entry.second.sock, &open_sockets_read);
                FD_SET(entry.second.sock, &open_sockets_write);
            }

            // Filter the socket lists and shift the readable resp. writeable ones to the front. Adjust length accordingly
            if(select(0, &open_sockets_read, &open_sockets_write, nullptr, &idle_timeout) == SOCKET_ERROR){
                std::cout << "Error checking sockets for readability and writeability. Error: " << WSAGetLastError() << std::endl;
                std::exit(1);
            }

            // Handle the readable sockets
            for(int i = 0; i < open_sockets_read.fd_count; i++){
                //std::cout << "Socket ready for reading at index " << i << ": " << open_sockets_read.fd_array[i] << std::endl;

                // The listening socket
                if(open_sockets_read.fd_array[i] == listening_socket){
                    SOCKET new_socket = accept(listening_socket, nullptr, nullptr);

                    // Handle different error types
                    if(new_socket == INVALID_SOCKET){
                        auto error = WSAGetLastError();
                        if(error == WSAEWOULDBLOCK){
                            std::cout << "Empty accept queue" << std::endl;
                        } else{
                            std::cout << "Error accepting from the accept queue. Error: " << error << std::endl;
                            continue;
                        }

                    // Add the new socket to connections
                    } else{
                        //std::cout << "Opened new socket: " << new_socket << std::endl;
                        auto _current_time = std::chrono::system_clock::now();
                        std::time_t current_time = std::chrono::system_clock::to_time_t(_current_time);
                        connections_map.insert({new_socket, {
                            new_socket, current_time
                        }});
                    }

                // The connected sockets
                } else{
                    int bytes_received = recv(open_sockets_read.fd_array[i], recv_buffer, recv_buffer_length, 0);

                    // Handle different error types
                    if(bytes_received == SOCKET_ERROR){
                        auto error = WSAGetLastError();
                        if(error == WSAEWOULDBLOCK){
                            continue;
                        } else{
                            std::cout << "Error recieving data from socket " << open_sockets_read.fd_array[i] << ". Error: " <<
                            error << ". Closing socket" << std::endl;
                            closesocket(open_sockets_read.fd_array[i]);
                            connections_map.erase(open_sockets_read.fd_array[i]);
                            continue;
                        }
                    
                    // i.e. TCP connection terminated
                    } else if(bytes_received == 0){
                        closesocket(open_sockets_read.fd_array[i]);
                        connections_map.erase(open_sockets_read.fd_array[i]);
                        std::cout << "Closed socket: " << open_sockets_read.fd_array[i] << std::endl;

                    // If data was successfully received
                    } else{
                        std::cout.write(recv_buffer, bytes_received);
                        update_socket_lastuse_connections_map(open_sockets_read.fd_array[i]);
                    }
                }
            }

            // Handle writeable sockets
            /*for(int i = 0; i < open_sockets_write.fd_count; i++){
                char send_buffer[64] = "dsuggers";
                //std::cout << "Socket ready for writing at index " << i << ": " << open_sockets_read.fd_array[i] << std::endl;
                int bytes_sent = send(open_sockets_write.fd_array[i], send_buffer, strlen(send_buffer), 0);

                // Handle different error types
                if(bytes_sent == SOCKET_ERROR){
                    auto error = WSAGetLastError();
                    if(error == WSAEWOULDBLOCK){
                        continue;
                    } else{
                        std::cout << "Error sending data on socket " << open_sockets_read.fd_array[i] << ". Error: " <<
                        error << ". Closing socket" << std::endl;
                        closesocket(open_sockets_write.fd_array[i]);
                        remove_socket_from_connections(open_sockets_write.fd_array[i]);
                        continue;
                    }
                } else{
                    std::cout << "Sent this many bytes: " << bytes_sent << std::endl;
                    update_socket_lastuse_connections(open_sockets_write.fd_array[i]);
                }
            } */
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

    void update_socket_lastuse_connections_map(SOCKET sock){
        auto _current_time = std::chrono::system_clock::now();
        std::time_t current_time = std::chrono::system_clock::to_time_t(_current_time);
        connections_map[sock].lastuse_time = current_time;
    }

    // Contains all current open TCP connections, and certain properties of them
    std::unordered_map<SOCKET, SOCKET_AND_PROPS> connections_map;

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