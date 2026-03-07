#pragma once
#include "config.h"
#include <chrono>
#include <functional>
#include <iostream>

namespace SetTimeout {
    enum CallbackType {
        Generic,
        PostWSARecv
    };

    class Timeout {
    public:
        void execute_callback(){
            callbackfn();
        }

        int callbacktype;
        std::chrono::steady_clock::time_point expiry;

        Timeout(std::function<void()> cb, int cbtype, int ms){
            expiry = std::chrono::steady_clock::now() + std::chrono::milliseconds(ms);
            callbackfn = cb;
            callbacktype = cbtype;
        }
    
    private:
        std::function<void()> callbackfn;
    };
}