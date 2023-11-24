#include "db_handler.h"
#include "server.h"

#define PORT 5556

#pragma comment(lib, "Ws2_32.lib")

int main(int argc, char** argv) {
    AuthServer server{PORT};
    server.RunLoop();

    return 0;
}