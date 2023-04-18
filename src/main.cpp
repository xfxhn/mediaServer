

#include "server/server.h"


int main() {
    int ret;
    constexpr unsigned short SERVER_RTSP_PORT = 8554;
    constexpr unsigned short SERVER_HTTP_PORT = 8080;
    Server server;

    ret = server.init(SERVER_RTSP_PORT, SERVER_HTTP_PORT);
    if (ret < 0) {
        return ret;
    }

    server.start();

    return 0;
}