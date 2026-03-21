#include "config.h"
#include "utilities.h"
#include "definitions.h"
#include "timeout.h"
#include "httpparsing.h"
#include "httpresponses.h"
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
    bool wsarecv_no_retry = false;
};

struct CmapShard{
    std::mutex mtx;
    std::unordered_map<SOCKET, SOCKET_INFO> shard;
};

class ShardedCmap{
private:
    int GetShardIndex(SOCKET sock){
        return Utils::hashint(sock, CMAP_NUM_SHARDS);
    }

    std::vector<CmapShard> shards{CMAP_NUM_SHARDS};

public:
    CmapShard * GetShard(SOCKET sock){
        const int sh = GetShardIndex(sock);
        return &shards[sh];
    }
};

struct TimeoutList{
    std::mutex mtx;
    std::vector<SetTimeout::Timeout> timeouts;
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
    SOCKET sock;
    WSABUF bufs[1];
    char recvbuf[RECV_BUFFER_SIZE];

    RecvContext(){
        bufs[0].buf = recvbuf;
        bufs[0].len = sizeof(recvbuf);
    }
};

struct SendContext{
    SOCKET sock;
    WSABUF bufs[1];
    char sendbuf[SEND_BUFFER_SIZE];

    SendContext(){
        bufs[0].buf = sendbuf;
        bufs[0].len = sizeof(sendbuf);
    }
};

class Fepttp{
public:
    Fepttp(int PORT){
        StartServer(PORT);
    }

    ~Fepttp(){

    }

    void run(){
        SYSTEM_INFO sysinfo{}; GetSystemInfo(&sysinfo); auto worker_count = 2 * sysinfo.dwNumberOfProcessors;
        for(int i = 0; i < worker_count; i++){
            workers.emplace_back([this]() { this->WorkerFunction((LPVOID)IOCP); });
        }

        for(int i = 0; i < CONCURRENT_ACCEPTEXS; i++){ PostAcceptEx(); posted_acceptexs++; }

        // Housekeeping loop
        while(true){
            Sleep(10);
            // Maintain the AcceptExs
            for(int i = posted_acceptexs; i < CONCURRENT_ACCEPTEXS; i++){
                if(PostAcceptEx() == IO_POST_SUCCESS){ posted_acceptexs++; }
            }

            // Handle timeouts
            auto now = std::chrono::steady_clock::now();
            for(auto& t : tl.timeouts){
                if(now >= t.expiry){
                t.ExecuteCallback();
                switch(t.callbacktype){
                case SetTimeout::CallbackType::Generic: // ---------- Generic callback ----------
                { break; }

                case SetTimeout::CallbackType::PostWSARecv: // ---------- WSARecv post callback ----------
                {
                CmapShard * sh = scmap.GetShard(t.sock);
                if(!sh->shard[t.sock].wsarecv_no_retry){ RepeatTimeout(t, IO_RETRY_DELAY_MS); }
                break;
                }

                default: { break; } // --------------------
                }
                }
            }

            // Erase the completed timeouts
            {   // <<<--------------->>>
            std::scoped_lock lock(tl.mtx);
            tl.timeouts.erase(
                std::remove_if(tl.timeouts.begin(), tl.timeouts.end(), [now](const SetTimeout::Timeout& t) { return (now >= t.expiry); }),
                tl.timeouts.end()
            );
            }   // <<<--------------->>>
        }
    }

private:
    WSADATA wsa_data;
    SOCKET lsocket;
    HANDLE IOCP;
    int worker_count;
    DWORD flags = 0;
    std::atomic<int> posted_acceptexs = 0;

    ShardedCmap scmap;
    
    TimeoutList tl;

    std::vector<std::thread> workers;

    void RepeatTimeout(SetTimeout::Timeout t, int ms){
        SetTimeout::Timeout tt = SetTimeout::RecreateTimeout(t, ms);
        { std::scoped_lock lock(tl.mtx); tl.timeouts.push_back(tt); }
    }

    int PostAcceptEx(){
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

    int PostWSARecv(SOCKET sock){
        RecvContext * ctx = new RecvContext();
        IOContextPtr * ctxptr = new IOContextPtr();
        ctxptr->ptr = (void *)ctx;
        ctxptr->optype = OpType::Recv;
        ctx->sock = sock;
        flags = 0;
        int success = WSARecv(
            sock,                        // The socket to receive from
            ctx->bufs,                   // The array of receive buffers
            1,                           // The number of buffers
            NULL,                        // Number of bytes received from the socket
            &flags,                      // A pointer to flags that can modify the behaviour of WSARecv
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

    void ScheduleRetryPostWSARecv(SOCKET sock){
        // Create a timeout to retry the WSARecv post which sets the flag to true if succesful
        SetTimeout::Timeout t([sock, this](){
            if(PostWSARecv(sock) == IO_POST_SUCCESS){
                {   // <<<--------------->>>
                CmapShard * sh = scmap.GetShard(sock); std::scoped_lock lock(sh->mtx);
                sh->shard[sock].wsarecv_no_retry = true;
                }   // <<<--------------->>>
            };

        }, SetTimeout::CallbackType::PostWSARecv, IO_RETRY_DELAY_MS, sock);

        // Set the timeout
        { std::scoped_lock lock(tl.mtx); tl.timeouts.push_back(t); }
    }

    int PostWSASend(SOCKET sock){
        RecvContext * ctx = new RecvContext();
        IOContextPtr * ctxptr = new IOContextPtr();
        ctxptr->ptr = (void *)ctx;
        ctxptr->optype = OpType::Send;
        ctx->sock = sock;
        flags = 0;
        int success = WSASend(
            sock,
            ctx->bufs,
            1,
            NULL,
            flags,
            &ctxptr->ov,
            NULL
        );

        if(success == SOCKET_ERROR){
            auto err = WSAGetLastError();
            if(err != ERROR_IO_PENDING){
                std::cout << "Error with WSASend. Error: " << err << std::endl;
                closesocket(sock); delete ctx; delete ctxptr;
                return IO_POST_FAILURE;
            }
        }

        return IO_POST_SUCCESS;
    }

    DWORD WINAPI WorkerFunction(LPVOID iocpptr){
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
        //std::cout << num_removed << std::endl;
        for(int i = 0; i < num_removed; i++){
            OVERLAPPED * ovptr = entries[i].lpOverlapped;
            IOContextPtr * ctxptr = CONTAINING_RECORD(ovptr, IOContextPtr, ov);

            switch(ctxptr->optype){
            case OpType::Accept: // ------------------------------ Accept operation -------------------------------------------
            {
            posted_acceptexs--;

            AcceptContext * ctx = (AcceptContext *)ctxptr->ptr; delete ctxptr;
            CmapShard * sh = scmap.GetShard(ctx->sock);

            if(PostAcceptEx() == IO_POST_SUCCESS){ posted_acceptexs++; }

            if(!CreateIoCompletionPort((HANDLE)ctx->sock, iocp, 0, 0)){
                std::cout << "Error associating socket " << ctx->sock << " to IOCP. Error: " << WSAGetLastError() << std::endl;
                delete ctx; continue;
            }

            std::cout << "Socket " << ctx->sock << " connected and associated to IOCP" << std::endl;

            {   // <<<--------------->>>
            std::scoped_lock lock(sh->mtx);
            sh->shard.insert({ctx->sock, {""}});
            }   // <<<--------------->>>

            if(PostWSARecv(ctx->sock) == IO_POST_FAILURE){ ScheduleRetryPostWSARecv(ctx->sock); }

            delete ctx;
            break;
            }

            case OpType::Recv: // --------------------------------- Recv operation --------------------------------------------
            {
            RecvContext * ctx = (RecvContext *)ctxptr->ptr; delete ctxptr;
            CmapShard * sh = scmap.GetShard(ctx->sock); 
            auto bytes = entries[i].dwNumberOfBytesTransferred;

            {   // <<<--------------->>>
            std::scoped_lock lock(sh->mtx);
            sh->shard[ctx->sock].wsarecv_no_retry = false;
            }   // <<<--------------->>>

            if(bytes == 0){
                const auto result = closesocket(ctx->sock);
                if(result == SOCKET_ERROR){
                    auto err = WSAGetLastError();
                    if(err != WSAENOTSOCK){
                        std::cout << "Error closing socket " << ctx->sock << std::endl;
                        delete ctx; continue;
                    }
                }
                {
                    std::scoped_lock lock(sh->mtx);
                    sh->shard.erase(ctx->sock);
                }
                std::cout << "Socket gracefully closed: " << ctx->sock << std::endl;
                delete ctx; continue;
            }

            if(PostWSARecv(ctx->sock) == IO_POST_FAILURE){ ScheduleRetryPostWSARecv(ctx->sock); }

            {   // <<<--------------->>>
            std::scoped_lock lock(sh->mtx);
            sh->shard[ctx->sock].reqacc.append(ctx->bufs[0].buf, bytes);
            }   // <<<--------------->>>

            int req_length = parser.RequestLength(sh->shard[ctx->sock].reqacc);
            if(req_length == CONTINUE){ delete ctx; continue; }
            if(req_length == CLOSE_ERASE_CONTINUE){
                delete ctx; closesocket(ctx->sock);
                {   // <<<--------------->>>
                    std::scoped_lock lock(sh->mtx);
                    sh->shard.erase(ctx->sock);
                }   // <<<--------------->>>
                continue;
            }

            std::cout << sh->shard[ctx->sock].reqacc.substr(0, req_length) << std::endl;
            HTTPRequest req = parser.ParseRequest(sh->shard[ctx->sock].reqacc);
            if(req.error == REQUEST_MALFORMED){
                delete ctx; closesocket(ctx->sock);
                {   // <<<--------------->>>
                    std::scoped_lock lock(sh->mtx);
                    sh->shard.erase(ctx->sock);
                }   // <<<--------------->>>
                continue;
            }

            {   // <<<--------------->>>
            std::scoped_lock lock(sh->mtx);
            sh->shard[ctx->sock].reqacc.erase(0, req_length);
            }   // <<<--------------->>>

            delete ctx;
            break;
            }

            case OpType::Send: // --------------------------------- Send operation --------------------------------------------
            {
            SendContext * ctx = (SendContext *)ctxptr->ptr; delete ctxptr;
            CmapShard * sh = scmap.GetShard(ctx->sock);
            auto bytes = entries[i].dwNumberOfBytesTransferred;

            std::cout << "Send operation completed on socket " << ctx->sock << std::endl;

            break;
            }
            }
        }
    }

    return 0;
    }

    int StartServer(int PORT){
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