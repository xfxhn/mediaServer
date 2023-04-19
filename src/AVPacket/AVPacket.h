
#ifndef RTSP_PACKET_H
#define RTSP_PACKET_H

#include <string>
#include <cstdint>
#include <fstream>
#include <chrono>
#include "transportStream/demuxPacket.h"
#include "nalu/NALReader.h"
#include "adts/adtsReader.h"
#include "adts/adtsHeader.h"

#define TRANSPORT_STREAM_PACKETS_SIZE 188

#define AVERROR_EOF (-10)

struct AVPackage {
    bool idrFlag{false};
    std::string type;
    double fps{0};
    uint64_t dts{0};
    uint64_t pts{0};
    uint32_t size{0};
    uint32_t decodeFrameNumber{0};
    std::vector<Frame> data1;
    uint8_t *data2{nullptr};
};

class AVPacket {
private:
    std::chrono::time_point<std::chrono::high_resolution_clock> start;
    std::chrono::duration<uint64_t, std::milli> elapsed;
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
    NALSeqParameterSet sps;
    NALPictureParameterSet pps;
    uint8_t *spsData{nullptr};
    uint8_t *ppsData{nullptr};
    uint8_t spsSize{0};
    uint8_t ppsSize{0};
    AdtsHeader aacHeader;
public:

    int init(const std::string &dir, uint32_t transportStreamPacketNumber);

    int getParameter();

    static AVPackage *allocPacket();

    static void freePacket(AVPackage *package);

    int readFrame(AVPackage *package);

    ~AVPacket();

private:
    int getTransportStreamData(bool videoFlag = true, bool audioFlag = true);
};


#endif //RTSP_PACKET_H
