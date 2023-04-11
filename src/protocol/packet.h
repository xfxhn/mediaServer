
#ifndef RTSP_PACKET_H
#define RTSP_PACKET_H

#include <string>
#include <cstdint>
#include <fstream>
#include <chrono>
#include "demuxPacket.h"
#include "NALReader.h"
#include "adtsReader.h"
#include "adtsHeader.h"

#define TRANSPORT_STREAM_PACKETS_SIZE 188

struct Packet {
    std::string type;
    uint64_t dts{0};
    uint64_t pts{0};
    std::vector<Frame> data1;
    uint8_t *data2{nullptr};
    uint32_t size{0};
};

class AVPacket {
private:
    std::chrono::time_point<std::chrono::high_resolution_clock> start;
    std::ifstream fs;
    std::string path;
    uint32_t currentPacket;

    uint8_t transportStreamBuffer[TRANSPORT_STREAM_PACKETS_SIZE];
    DemuxPacket demux;


    NALReader videoReader;
    NALPicture *picture;
    AdtsReader audioReader;
    AdtsHeader header;
public:

    int test();

    int init(const std::string &dir, uint32_t transportStreamPacketNumber);

    static Packet *allocPacket();

    static void freePacket(Packet *packet);

    int readFrame(Packet *packet);

private:
    int getTransportStreamData();
};


#endif //RTSP_PACKET_H
