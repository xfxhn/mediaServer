

#include "server.h"
#include "utils/util.h"
/*
//#include <cstdint>
//#include <iostream>
//#include "readStream.h"
//
//#include "AVPacket.h"
//#include "NALReader.h"
//#include <filesystem>*/
#include <fstream>

int main() {
    int ret;
    Server server;

    server.init();

    server.start();

    return 0;
}