#include "fepttp.h"
#include <iostream>

int main(){
    //

    Fepttp server = Fepttp(8000);
    server.run();

    return 0;
}