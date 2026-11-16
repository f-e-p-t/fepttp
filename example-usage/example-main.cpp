#include "../fepttp.h"
#include "example-middleware.h"

int main(){

    Fepttp server = Fepttp(8000);

    server.chain.push_back(JohnMiddleware);

    server.run();

    return 0;
}