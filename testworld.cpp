#include <iostream>
#include <mutex>

class Dsuga {
    std::mutex mtx;
};

int main(){
    int x = 1;

    switch(x){
        case 0: break;
        case 1: std::cout << "bruh" << std::endl; break;
        case 2: break;
    }

    return 0;
}