

#include "server/server.h"


int main() {

    int ret;
    constexpr unsigned short SERVER_RTSP_PORT = 554;
    constexpr unsigned short SERVER_HTTP_PORT = 80;

    Server server;
    ret = server.init(SERVER_RTSP_PORT, SERVER_HTTP_PORT);
    if (ret < 0) {
        return ret;
    }

    server.start();

    return 0;
}