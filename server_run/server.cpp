#include <iostream>
#include "../server_core/include/ServerCore.h"




int main() {
    std::cout << "\033[1;32mStarted a work of server_core\033[0m" << std::endl;

    ServerCore server;
    server.start();
    server.joinLoop();

    return 0;
}
