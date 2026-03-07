#pragma once
#include "config.h"
#include <chrono>
#include <functional>
#include <iostream>

namespace SetTimeout {
    class Timeout {
    public:
        void execute_callback(){
            callbackfn();
        }

        std::chrono::steady_clock::time_point expiry;

        Timeout(std::function<void()> cb, int ms){
            expiry = std::chrono::steady_clock::now() + std::chrono::milliseconds(ms);
            callbackfn = cb;
        }
    
    private:
        std::function<void()> callbackfn;
    };
}