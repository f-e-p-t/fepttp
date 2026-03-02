#include "config.h"
#include "utilities.h"
#include "definitions.h"
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
#include <mutex>
#include <thread>
#include <atomic>
#include <iostream>

using namespace Config;
using namespace Definitions;

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "mswsock.lib")
#pragma comment(lib, "kernel32.lib")

enum OpType { Accept, Recv, Send };

struct SOCKET_INFO {
    std::string reqacc;
};

struct CmapShard {
    std::mutex mtx;
    std::unordered_map<SOCKET, SOCKET_INFO> shard;
};

class ShardedCmap{
private:
    int get_shard_index(SOCKET sock){
        return hashint(sock, CMAP_NUM_SHARDS);
    }

    std::vector<CmapShard> shards{CMAP_NUM_SHARDS};

public:
    CmapShard * get_shard(SOCKET sock){
        const int sh = get_shard_index(sock);
        return &shards[sh];
    }

};

struct IOContextPtr{
    int optype;
    OVERLAPPED ov{};
    void * ptr;
};

struct AcceptContext{
    SOCKET sock = INVALID_SOCKET;
    char addr_buf[(sizeof(sockaddr_in) + 16) * 2];
};

struct RecvContext{
    WSABUF bufs[1];
    char recvbuf[RECV_BUFFER_SIZE];
    DWORD bytes;

    RecvContext(){
        bufs[0].buf = recvbuf;
        bufs[0].len = sizeof(recvbuf);
    }
};

struct SendContext{
    // Not in use yet
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

        SYSTEM_INFO sysinfo{}; GetSystemInfo(&sysinfo); auto worker_count = 2 * sysinfo.dwNumberOfProcessors;
        for(int i = 0; i < worker_count; i++){
            workers.emplace_back([this]() { this->worker_function((LPVOID)IOCP); });
        }

        for(int i = 0; i < CONCURRENT_ACCEPTEXS; i++){ post_AcceptEx(); posted_acceptexs++; }

        // Housekeeping loop
        while(true){
            Sleep(50);
            if(posted_acceptexs < CONCURRENT_ACCEPTEXS){
                if(post_AcceptEx() == IO_POST_SUCCESS){ posted_acceptexs++; }
            }
        }
    }

private:
    WSADATA wsa_data;
    SOCKET lsocket;
    HANDLE IOCP;
    int worker_count;
    std::atomic<int> posted_acceptexs = 0;

    ShardedCmap scmap;

    std::vector<std::thread> workers;

    int post_AcceptEx(){
        AcceptContext * ctx = new AcceptContext();
        IOContextPtr * ctxptr = new IOContextPtr();
        ctxptr->ptr = (void *)ctx;
        ctxptr->optype = OpType::Accept;
        ctx->sock = WSASocket(AF_INET, SOCK_STREAM, 0, nullptr, 0, WSA_FLAG_OVERLAPPED);
        BOOL success = AcceptEx(
            lsocket,                      // The listening socket
            ctx->sock,                    // The socket to accept the connection on
            ctx->addr_buf,                // Where to store the remote and local addresses
            0,                            // Number of bytes of TCP stream data to initially receive
            sizeof(sockaddr_in) + 16,     // Number of bytes for the local address
            sizeof(sockaddr_in) + 16,     // Number of bytes for the remote address
            nullptr,                      // AcceptEx will set this to the number of bytes received (can find in OVERLAPPED)
            &ctxptr->ov                   // AcceptEx will send the completion packet here
        );

        if(success == FALSE){
            auto err = WSAGetLastError();
            if(err != ERROR_IO_PENDING){
                std::cout << "Error with AcceptEx. Error: " << err << std::endl; 
                closesocket(ctx->sock); delete ctx; delete ctxptr;
                return IO_POST_FAILURE;
            }
        }

        return IO_POST_SUCCESS;
    }

    // TEST THIS AND IMPLEMENT IT
    int post_WSARecv(SOCKET sock){
        RecvContext * ctx = new RecvContext();
        IOContextPtr * ctxptr = new IOContextPtr();
        ctxptr->ptr = (void *)ctx;
        ctxptr->optype = OpType::Recv;
        int success = WSARecv(
            sock,                        // The socket to receive from
            ctx->bufs,                   // The array of receive buffers
            1,                           // The number of buffers
            &ctx->bytes,                 // Number of bytes received from the socket
            nullptr,                     // A pointer to flags that can modify the behaviour of WSARecv
            &ctxptr->ov,                 // WSARecv will send the completion packet here
            NULL                         // Completion routine
        );

        if(success == SOCKET_ERROR){
            auto err = WSAGetLastError();
            if(err != ERROR_IO_PENDING){
                std::cout << "Error with WSARecv. Error: " << err << std::endl;
                closesocket(sock); delete ctx; delete ctxptr;
                return IO_POST_FAILURE;
            }
        }

        return IO_POST_SUCCESS;
    }

    // The event loop for each worker thread, which takes the IOCP as a parameter. Needs to be static to give a plain function ptr
    DWORD WINAPI worker_function(LPVOID iocpptr){
    HANDLE iocp = (HANDLE)iocpptr;
    while(true){
        OVERLAPPED_ENTRY entries[GQCSEX_MAX_INTAKE];
        ULONG num_removed;

        if(!GetQueuedCompletionStatusEx(iocp, entries, GQCSEX_MAX_INTAKE, &num_removed, INFINITE, FALSE)){
            const auto err = WSAGetLastError();
            std::cout << "GQCSEx error: " << err << std::endl;
            if(err == ERROR_ABANDONED_WAIT_0){ break; /* IOCP gone. All threads will shut down */ }
            continue;
        }
        std::cout << num_removed << std::endl;
        for(int i = 0; i < num_removed; i++){
            OVERLAPPED * ovptr = entries[i].lpOverlapped;
            IOContextPtr * ctxptr = CONTAINING_RECORD(ovptr, IOContextPtr, ov);
            switch(ctxptr->optype){
            case OpType::Accept: // ------------------------------ Accept operation -------------------------------------------
            {
            posted_acceptexs--;

            AcceptContext * ctx = (AcceptContext *)ctxptr->ptr;

            if(!CreateIoCompletionPort((HANDLE)ctx->sock, iocp, 0, 0)){
                std::cout << "Error associating socket " << ctx->sock << " to IOCP. Error: " << WSAGetLastError() << std::endl;
                delete ctx; continue;
            }

            std::cout << "Socket " << ctx->sock << " connected and associated to IOCP" << std::endl;

            {
                CmapShard * sh = scmap.get_shard(ctx->sock);
                std::scoped_lock lock(sh->mtx);
                sh->shard.insert({ctx->sock, {""}});
            }

            if(post_AcceptEx() == IO_POST_SUCCESS){ posted_acceptexs++; }

            std::cout << posted_acceptexs << std::endl;

            // NEXT: Post a single WSARecv and WSASend to the new socket. Maintain them

            delete ctx;

            break;
            }

            case OpType::Recv: // --------------------------------- Recv operation --------------------------------------------
            {
            break;
            }

            case OpType::Send: // --------------------------------- Send operation --------------------------------------------
            {
            break;
            }
            }

            delete ctxptr;
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