#include <iostream>
#include <winsock2.h>
#include <stdio.h>
#include <string>
#include <vector>uji9

class Fepttp{
public:
    Fepttp(int PORT){
        startServer(PORT);
    }

    ~Fepttp(){
        closeServer();
    }

    void run(){
        while(true){
            fd_set open_sockets_read; // Set of sockets to be checked for readability
            fd_set open_sockets_write; // ... writeability
            FD_ZERO(&open_sockets_read); // Clears both lists of sockets
            FD_ZERO(&open_sockets_write);
            FD_SET(listening_socket, &open_sockets_read); // Add the listening socket to the potentially readable set

            for(SOCKET sock : connections){
                FD_SET(sock, &open_sockets_read); // Fill up both lists from connections vector
                FD_SET(sock, &open_sockets_write);
            }

            // Parameter 5: time until thread is woken up automatically. Otherwise, it is only woken up by I/O
            // It will wake up to close the sockets whose idle timeout is expired (not implemented yet)
            // Think: 'when do I need to wake up even if the network is silent?'
            select(0, &open_sockets_read, &open_sockets_write, nullptr, nullptr); // CHANGE PARAM 5 FROM NULLPTR
        }
    }
private:
    SOCKET listening_socket;
    WSADATA _wsa;
    u_long iMode = 1; // Non-blocking mode
    sockaddr_in address;
    int bind_result;

    // CREATE A NEW STRUCT TO ATTACH LAST-ACTIVITY TAGS TOO EACH SOCKET. USE TO IMPLEMENT IDLE TIMEOUT
    std::vector<SOCKET> connections; // Contains all current open TCP connections

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