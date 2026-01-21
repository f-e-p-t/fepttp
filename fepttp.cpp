#include <iostream>
#include <winsock2.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <chrono>

#define IDLE_TIMEOUT_S 3;
#define IDLE_TIMEOUT_ADDITIONAL_MS 0;

struct SOCKET_AND_LASTUSE {
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
            fd_set open_sockets_read; // Set of sockets to be checked for readability
            fd_set open_sockets_write; // ... writeability
            FD_ZERO(&open_sockets_read); // Clears both lists of sockets
            FD_ZERO(&open_sockets_write);
            FD_SET(listening_socket, &open_sockets_read); // Add the listening socket to the potentially readable set

            for(SOCKET_AND_LASTUSE sock : connections){
                FD_SET(sock.sock, &open_sockets_read); // Fill up both lists from connections vector
                FD_SET(sock.sock, &open_sockets_write);
            }

            // Filter the socket lists and leave only those that are readable resp. writeable
            if(select(0, &open_sockets_read, &open_sockets_write, nullptr, &idle_timeout) == SOCKET_ERROR){
                std::cout << "Error checking sockets for readability and writeability. Error: " << WSAGetLastError() << std::endl;
                std::exit(1);
            }

            // Accept a new connection from the listening socket's accept queue, if there is one
            SOCKET new_socket = accept(listening_socket, nullptr, nullptr);
            if(new_socket == INVALID_SOCKET){
                auto error = WSAGetLastError();
                if(error == WSAEWOULDBLOCK){ // If the error is an empty queue
                    //std::cout << "Empty accept queue" << std::endl;
                } else{
                    std::cout << "Error accepting from the accept queue. Error: " << error << std::endl;
                }
            } else{
                std::cout << new_socket << std::endl;
            }
        }
    }
private:
    SOCKET listening_socket;
    WSADATA _wsa;
    u_long iMode = 1; // Non-blocking mode
    sockaddr_in address;
    int bind_result;
    timeval idle_timeout;

    std::vector<SOCKET_AND_LASTUSE> connections; // Contains all current open TCP connections, and the time of their last uses

    // Create the listening socket
    int startServer(int PORT){
        std::cout << "Starting server" << std::endl;

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
        if(ioctlsocket(listening_socket, FIONBIO, &iMode) != 0){
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