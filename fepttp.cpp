#include "config.h"
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <mswsock.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <chrono>
#include <algorithm>
#include <unordered_map>
#include <iostream>

using namespace Config;

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "mswsock.lib")
#pragma comment(lib, "kernel32.lib")

enum OpType { Accept, Recv, Send };

struct SOCKET_INFO {
    SOCKET sock;
    std::time_t lastuse;
    std::string acc;
};

// SPLIT THIS INTO MULTIPLE LATER (ACCEPT, RECV, SEND)
struct IOContext{
    int optype;
    SOCKET sock = INVALID_SOCKET; // Replaces completion key
    OVERLAPPED ov{};
    char addr_buffer[(sizeof(sockaddr_in) + 16) * 2];
    WSABUF wsabuf;
    char recvbuf[RECV_BUFFER_SIZE];
    char sendbuf[SEND_BUFFER_SIZE];
};

class Fepttp{
public:
    Fepttp(int PORT){
        start_server(PORT);
    }

    ~Fepttp(){

    }

    void run(){
        // TESTING PURPOSES ONLY. ACCEPTEX POSTS SHOULD BE AFTER THREAD SPAWNING
        //for(int i = 0; i < CONCURRENT_ACCEPTEXS; i++){ post_AcceptEx(); } Sleep(6000);

        // Set the number of worker threads to 2X the number of logical processors on the computer. Create the threads
        SYSTEM_INFO sysinfo{}; GetSystemInfo(&sysinfo); auto worker_count = 2 * sysinfo.dwNumberOfProcessors;
        for(int i = 0; i < worker_count; i++){
            CreateThread(NULL, 0, worker_function, (LPVOID)IOCP, 0, nullptr);
        }

        for(int i = 0; i < CONCURRENT_ACCEPTEXS; i++){ post_AcceptEx(); }

        // If run() finishes, the program exits in main(), so keep it alive, and doing something useful
        while(true){
            Sleep(1000);
        }
    }

private:
    WSADATA wsa_data;
    SOCKET lsocket;
    HANDLE IOCP;
    int worker_count;

    void post_AcceptEx(){ // No parameters since AcceptEx requests are only posted to the listening socket
        // This is a heap variable, so it won't go out of scope. IT WILL NEED TO BE DELETED MANUALLY IN THE WORKER THREAD
        // ADD THE ACCEPT SOCKET TO CMAP ONLY AFTER COMPLETION
        IOContext * ctx = new IOContext();
        ctx->optype = OpType::Accept;
        ctx->sock = WSASocket(AF_INET, SOCK_STREAM, 0, nullptr, 0, WSA_FLAG_OVERLAPPED);
        BOOL success = AcceptEx(
            lsocket,                      // The listening socket
            ctx->sock,                    // The socket to accept the connection on
            ctx->addr_buffer,             // Where to store the remote and local addresses
            0,                            // Number of bytes of TCP stream data to initially receive
            sizeof(sockaddr_in) + 16,     // Number of bytes for the local address
            sizeof(sockaddr_in) + 16,     // Number of bytes for the remote address
            nullptr,                     // AcceptEx will set this to the number of bytes received (can find in OVERLAPPED)
            &ctx->ov                      // AcceptEx will send the completion packet here
        );

        if(!success){
            auto err = WSAGetLastError();
            if(err != ERROR_IO_PENDING){
                std::cout << "Error with AcceptEx. Error: " << err << std::endl; closesocket(ctx->sock); delete ctx;
            }
        }
    }
 
    // The event loop for each worker thread, which takes the IOCP as a parameter. Needs to be static to give a plain function ptr
    static DWORD WINAPI worker_function(LPVOID iocpptr){
    HANDLE iocp = (HANDLE)iocpptr;
    while(true){
        OVERLAPPED_ENTRY entries[GQCSEX_MAX_INTAKE];
        ULONG num_removed;

        GetQueuedCompletionStatusEx(iocp, entries, GQCSEX_MAX_INTAKE, &num_removed, INFINITE, FALSE);

        std::cout << num_removed << std::endl;

        for(int i = 0; i < num_removed; i++){
            OVERLAPPED * ovptr = entries[i].lpOverlapped;
            IOContext * ctx = CONTAINING_RECORD(ovptr, IOContext, ov);
            switch(ctx->optype){
            case OpType::Accept: // ------------------------------ Accept operation -------------------------------------------

            if(!CreateIoCompletionPort((HANDLE)ctx->sock, iocp, 0, 0)){
                std::cout << "Error associating socket " << ctx->sock << " to iocp." << std::endl;
                break;
            }
            
            std::cout << "Socket " << ctx->sock << " connected and associated to iocp" << std::endl;

            // create socket object (cmap replacement), delete the ctx object when done with it, etc
            // maybe post an initial batch of WSARecv and WSASend as with AcceptEx

            // Hashmap leads to race conditions, so will need an object per open connection. Two threads can simultaneously read and
            // write on this socket, so race conditions are still a problem. Solutions include using a mutex or an atomic flag.
            // shouldnt be a problem while just accepting the connection, but will be for reads and writes.
            // Q: If a thread gets a completion packet from the IOCP and tries to edit the per-connection object but gets blocked by
            // a mutex, will the work never get done?
            // A: No, the thread pauses at the line that requires the mutex until it can get the mutex, then it proceeds.

            break;
            case OpType::Recv: // --------------------------------- Recv operation --------------------------------------------
            break;
            case OpType::Send: // --------------------------------- Send operation --------------------------------------------
            break;
            }
        }
    }

    return 0;
    }

    int start_server(int PORT){
        std::cout << "Starting server on port " << PORT << std::endl;

        // Initialise WSA
        if(WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0){
            std::cout << "WSA Startup failed. Error: " << WSAGetLastError() << std::endl; WSACleanup(); return 1;
        }

        // Create the listening socket
        lsocket = WSASocket(AF_INET, SOCK_STREAM, 0, nullptr, 0, WSA_FLAG_OVERLAPPED);
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

        // Create the IOCP. Second parameter is an existing IOCP. If this is not nullptr, then this function associates the handle
        // in the first parameter to the specified IOCP. We do this below with the listening socket. Parameter 3 is a completion
        // key. Parameter 4 is the number of worker threads. 0 sets it to default
        IOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
        if(!IOCP){
            std::cout << "Error creating IOCP. Error: " << WSAGetLastError() <<  std::endl; WSACleanup(); return 1;
        }

        if(!CreateIoCompletionPort((HANDLE)lsocket, IOCP, 0, 0)){
            std::cout << "Error associating lsocket to IOCP. Error: " << WSAGetLastError() << std::endl; WSACleanup(); return 1;
        }

        return 0;
    }
};

int main(){
    Fepttp server = Fepttp(8000);
    server.run();

    return 0;
}