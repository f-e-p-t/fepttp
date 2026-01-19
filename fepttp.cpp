#include <iostream>
#include <winsock2.h>
#include <stdio.h>
#include <string>

class Fepttp{
public:
    Fepttp(int PORT){
        startServer(PORT); // CONSTRUCTOR SHOULD NOT RUN THE EVENT LOOP.
    }

    ~Fepttp(){
        closeServer();
    }

    int run(){
        // Event loop here
    }
private:
    SOCKET listeningSocket;
    WSADATA _wsa;
    u_long iMode = 1; // Non-blocking
    sockaddr_in address;
    int bindResult;

    // Create the listening socket
    int startServer(int PORT){
        std::cout << "Starting server" << std::endl;

        // Initialise WSA
        if(WSAStartup(MAKEWORD(2, 2), &_wsa) != 0){
            std::cout << "WSA Startup failed. Error: " << WSAGetLastError() << std::endl;
            std::exit(1);
        }
        
        // Create socket to be listened on
        listeningSocket = socket(AF_INET, SOCK_STREAM, 0);
        if(listeningSocket == INVALID_SOCKET){
            std::cout << "Socket creation failed. Error: " << WSAGetLastError() << std::endl;
            std::exit(1);
        }

        // Set listening socket to non-blocking mode
        if(ioctlsocket(listeningSocket, FIONBIO, &iMode) != 0){
            std::cout << "ioctlsocket failed. Error: " << WSAGetLastError() << std::endl;
            std::exit(1);
        }

        // Set address
        address.sin_family = AF_INET;
        address.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
        address.sin_port = htons(PORT);

        // Bind listening socket to a port
        bindResult = bind(listeningSocket, (SOCKADDR*) &address, sizeof(address));
        if(bindResult == SOCKET_ERROR){
            std::cout << "Bind listening socket failed. Error: " << WSAGetLastError() << std::endl;
            bindResult = closesocket(listeningSocket);
            if(bindResult == SOCKET_ERROR){
                std::cout << "Close listening socket failed. Error: " << WSAGetLastError() << std::endl;
            }
            std::exit(1);
        }

        if(listen(listeningSocket, SOMAXCONN) == SOCKET_ERROR){ // SOMAXCONN is the maximum backlog (connection queue) size
            std::cout << "Listen failed. Error: " << WSAGetLastError();
            std::exit(1);
        }

        return 0;
    }

    int closeServer(){
        closesocket(listeningSocket); WSACleanup();
        return 0;
    }
};

int main(){
    Fepttp app = Fepttp(8000);

    app.run();

    return 0;
}