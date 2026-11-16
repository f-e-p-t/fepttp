#pragma once
#include <string>

namespace Config {
    constexpr bool PRINT_NEW_SOCKETS = false;   // Print info about new socket connections to the terminal
    constexpr bool PRINT_GRACEFUL_CLOSURES = false;   // Print info about graceful socket closures to the terminal
    constexpr bool PRINT_REQS = false;          // Print incoming requests to the terminal
    constexpr bool PRINT_SENDS = false;         // Print info about completed send operations to the terminal
    constexpr bool PRINT_TIMEOUTS = false;      // Print info about socket timeouts to the terminal     

    constexpr int HOUSEKEEPER_POLL_RATE_MS = 10;   // Time (in ms) between each iteration of the housekeeping loop

    constexpr int CONCURRENT_ACCEPTEXS = 256;   // There will always be this many AcceptExs waiting to be completed. This is achieved by
                                                // posting this many when the run() method is called, and then replenishing an AcceptEx
                                                // request whenever one is dealt with in the worker threads. Reasoning: deal with batches
                                                // of incoming connections instantly
    
    constexpr int GQCSEX_MAX_INTAKE = 256;      // A cap on the number of completion packets that GetQueuedCompletionStatusEx can
                                                // take in at once

    constexpr int RECV_BUFFER_SIZE = 4096;        // The size (in bytes) of the WSARecv buffer

    constexpr int SEND_BUFFER_SIZE = 4096;        // Similarly for the WSASend buffer

    constexpr int CMAP_NUM_SHARDS = 256;        // The number of shards in the sharded connection maps. Set to 1? A single shard,
                                                // hence a single mutex that every worker thread fights over. Set too high? Too many
                                                // mutexes, so too much memory wasted. THIS MUST BE A POWER OF 2

    constexpr int IO_RETRY_DELAY_MS = 50;     // If posting an IO operation (for example, WSARecv) fails, the server will wait
                                                // this many milliseconds before retrying the operation

    constexpr int REQ_HEADER_CAP = 65536;       // Reject incoming requests if their headers exceed this length

    constexpr int REQ_BODY_CAP = 65536;         // Same for request bodies

    constexpr int IDLE_TIMEOUT_S = 15;          // Time (in seconds) a socket may stay open before being closed automatically

    constexpr int TIMEOUT_SWEEPER_POLL_RATE_MS = 5000;   // Time (in ms) between expiry checks on all open sockets
}