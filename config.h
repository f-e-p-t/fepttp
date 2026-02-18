#pragma once
#include <string>

namespace Config {
    constexpr int CONCURRENT_ACCEPTEXS = 256; // There will always be this many AcceptExs waiting to be completed. This is achieved by
                                              // posting this many when the run() method is called, and then replenishing an AcceptEx
                                              // request whenever one is dealt with in the worker threads. Reasoning: deal with batches
                                              // of incoming connections instantly
}