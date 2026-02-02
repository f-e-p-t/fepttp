#pragma once
#include <string>

namespace Config{
    constexpr int IDLE_TIMEOUT_S = 5;
    constexpr int IDLE_TIMEOUT_ADDITIONAL_MS = 0;
    constexpr int RECV_BUFFER_LENGTH = 64;
    constexpr int SEND_BUFFER_LENGTH = 64;
    constexpr int REQ_HEADER_LENGTH_CAP = 16384;
    constexpr int REQ_BODY_LENGTH_CAP = 16384;
}