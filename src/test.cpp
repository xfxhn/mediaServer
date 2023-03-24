

#include "server.h"
#include "utils/util.h"
#include <cstdint>
#include <iostream>
#include "readStream.h"


#include "NALReader.h"

int main() {


// Your function here

    std::chrono::time_point<std::chrono::high_resolution_clock> end;
    std::chrono::duration<double, std::milli> elapsed{};

    auto start = std::chrono::high_resolution_clock::now();
    /*while (true) {
        for (int i = 0; i < 1000000; ++i) {
            int a = 1;
        }

        elapsed = std::chrono::high_resolution_clock::now() - start;

        std::cout << "Elapsed time: " << elapsed.count() << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds((int) 1000));
        start = std::chrono::high_resolution_clock::now();
    }*/
    /*int ret1;
    NALReader reader;
    reader.init1("test/", 0);
    uint64_t aaa = av_rescale_q(1, {1, 30}, {1, 1000});
    uint8_t *data;
    uint32_t size;
    while (true) {
        ret1 = reader.readNalUint1(data, size);
        if (ret1 < 0) {
            return ret1;
        }
    }


    return 0;*/


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