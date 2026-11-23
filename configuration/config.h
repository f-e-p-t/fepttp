#ifndef SERVER_CONFIG
#define SERVER_CONFIG

namespace Config {
    // Print info about new socket connections to the terminal
    constexpr bool PRINT_NEW_SOCKETS = true;

    // Print info about graceful socket closures to the terminal
    constexpr bool PRINT_GRACEFUL_CLOSURES = true;

    // Print incoming requests to the terminal
    constexpr bool PRINT_REQS = false;

    // Print info about completed send operations to the terminal
    constexpr bool PRINT_SENDS = false;

    // Print info about socket timeouts to the terminal
    constexpr bool PRINT_TIMEOUTS = true;  

    // Time (in ms) between each iteration of the housekeeping loop
    constexpr int HOUSEKEEPER_POLL_RATE_MS = 10;

    // There will always be this many AcceptExs waiting to be completed
    constexpr int CONCURRENT_ACCEPTEXS = 256;

    // The maximum number of completion packets GetQueuedCompletionStatusEx may take in at once
    constexpr int GQCSEX_MAX_INTAKE = 256;

    // The size of the WSARecv buffer in bytes
    constexpr int RECV_BUFFER_SIZE = 4096;

    // The size of the WSASend buffer in bytes
    constexpr int SEND_BUFFER_SIZE = 4096;

    // The number of shards in the sharded connections map (must be a power of 2)
    constexpr int CMAP_NUM_SHARDS = 256;

    // If posting an IO operation fails, the server waits this many ms before retrying the operation
    constexpr int IO_RETRY_DELAY_MS = 50;

    // Reject incoming requests if their headers have more than this many bytes
    constexpr int REQ_HEADER_CAP = 65536;

    // Reject incoming requests if their bodies have more than this many bytes
    constexpr int REQ_BODY_CAP = 65536;

    // An idle socket is automatically closed after this many milliseconds
    constexpr int IDLE_TIMEOUT_MS = 15000;

    // Sockets are checked for idle timeout periodically with this period
    constexpr int TIMEOUT_SWEEPER_POLL_RATE_MS = 5000;
}

#endif