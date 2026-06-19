#include "fepttp.h"
#include <iostream>

int main(){

    Fepttp server = Fepttp(8000);

    server.chain.push_back(AddHeaderToReq);
    server.chain.push_back(DetectThatHeaderText);
    server.chain.push_back(JaneMiddleware);
    server.chain.push_back(JohnMiddleware);

    server.run();

    return 0;
}