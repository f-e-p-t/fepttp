#pragma once
#include <string>

namespace Config {
    constexpr int SELECT_TIMEOUT_S = 5;
    constexpr int SELECT_TIMEOUT_MS = 0; // In addition to the seconds
    constexpr int RECV_BUFFER_LENGTH = 64;
    constexpr int SEND_BUFFER_LENGTH = 64;
    constexpr int REQ_HEADER_LENGTH_CAP = 16384;
    constexpr int REQ_BODY_LENGTH_CAP = 16384;
    constexpr int SOCKET_IDLE_TIMEOUT_S = 5;
}