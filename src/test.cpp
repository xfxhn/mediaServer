

#include "server.h"
#include "utils/util.h"
#include <cstdint>
#include <iostream>
#include "readStream.h"

int main() {
    uint8_t arr[12] = {0x80, 0xe0, 0x42, 0x24, 0x59, 0x1a, 0x7b, 0x45, 0x80};
    ReadStream rs(arr, 12);


    int version = rs.readMultiBit(2);
    int padding = rs.readMultiBit(1);
    int extension = rs.readMultiBit(1);
    if (extension) {
        fprintf(stderr, "不支持扩展包头\n");
        return -1;
    }
    int csrcLen = rs.readMultiBit(4);
    int marker = rs.readMultiBit(1);
    int payloadType = rs.readMultiBit(7);

    std::string aaa = decimalToHex(12);
    int ret;
    Server server;

    server.init();
//    ret = rp.initVideo("./resource/slice3gop2.h264");
//    if (ret < 0) {
//        return ret;
//    }
//    ret = rp.initAudio("./resource/ouput1.aac");
//    if (ret < 0) {
//        return ret;
//    }
    server.start();
    //rp.sendAudio("./resource/ouput1.aac", "127.0.0.1", 9832);


    /*TcpSocket sock;
    sock.init();


    sock.createSocket();
    sock.bindSocket("0.0.0.0", 8554);
    sock.listenPort();
    sock.wait();*/

    return 0;
}