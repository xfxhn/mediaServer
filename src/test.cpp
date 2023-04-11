

#include "server.h"
#include "utils/util.h"
#include <cstdint>
#include <iostream>
#include "readStream.h"

#include "packet.h"
#include "NALReader.h"
#include <filesystem>

int main() {
    AVPacket packet;
    packet.init("./live/test", 0);
    packet.test();
    return 0;
    int ret;
    Server server;

    server.init();

    server.start();

    return 0;
}