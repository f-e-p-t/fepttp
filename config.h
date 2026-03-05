#pragma once
#include <string>

namespace Config {
    constexpr int CONCURRENT_ACCEPTEXS = 256;   // There will always be this many AcceptExs waiting to be completed. This is achieved by
                                                // posting this many when the run() method is called, and then replenishing an AcceptEx
                                                // request whenever one is dealt with in the worker threads. Reasoning: deal with batches
                                                // of incoming connections instantly
    
    constexpr int CONCURRENT_WSARECVS = 10;     // As above but with WSARecv. Larger buffer --> less. Smaller buffer --> more.

    constexpr int GQCSEX_MAX_INTAKE = 256;      // A cap on the number of completion packets that GetQueuedCompletionStatusEx can
                                                // take in at once

    constexpr int RECV_BUFFER_SIZE = 512;       // The size (in bytes) of the WSARecv buffer

    constexpr int SEND_BUFFER_SIZE = 64;        // Similarly for the WSASend buffer

    constexpr int CMAP_NUM_SHARDS = 16;         // The number of shards in the sharded connections map. Set to 1? A single shard,
                                                // hence a single mutex that every worker thread fights over. Set too high? Too many
                                                // mutexes, so too much memory wasted. THIS MUST BE A POWER OF 2
}