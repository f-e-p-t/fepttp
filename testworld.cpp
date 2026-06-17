#include <iostream>
#include <mutex>
#include <string>
#include <string_view>
#include <charconv>
#include <unordered_map>
#include <thread>
#include <winsock2.h>


std::unordered_map<int, int> hashmap;
std::mutex mtx;

void infinite_loop(){

    while(true){
        Sleep(250);
        {
        std::scoped_lock lock(mtx);
        hashmap[500] = 4;
        std::cout << "t " << std::endl;
        }
    }
}

int main(){
    for(int i = 0; i < 1000; i++){
        hashmap[i] = i * i;
    }

    std::thread th = std::thread(infinite_loop);
    //th.join();

    Sleep(2000);
    {
    std::scoped_lock lock(mtx);

    }   

    while(true){}

    return 0;
}