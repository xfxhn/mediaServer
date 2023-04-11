
#ifndef MUX_NALREADER_H
#define MUX_NALREADER_H

#include <cstdint>
#include <fstream>
#include <cstring>
#include "NALHeader.h"
#include "NALPicture.h"
#include "NALDecodedPictureBuffer.h"


#include "demuxPacket.h"

#define TRANSPORT_STREAM_PACKETS_SIZE 188

enum NalUintType {
    H264_NAL_UNSPECIFIED = 0,
    H264_NAL_SLICE = 1,
    H264_NAL_IDR_SLICE = 5,
    H264_NAL_SEI = 6,
    H264_NAL_SPS = 7,
    H264_NAL_PPS = 8
};

class NALReader {
public:

    static constexpr uint32_t MAX_BUFFER_SIZE{1024 * 1024};
    /*bool pictureFinishFlag{ false };*/
    NALSeqParameterSet sps;
    NALPictureParameterSet pps;
    uint8_t spsData[50];
    uint8_t ppsData[50];
    uint8_t spsSize{0};
    uint8_t ppsSize{0};
    /*uint32_t remainingByte{0};*/


    uint8_t *bufferStart{nullptr};
    uint8_t *bufferPosition{nullptr};
    uint8_t *bufferEnd{nullptr};
    uint32_t blockBufferSize{0};
private:

    std::string path;

//    NALSliceHeader sliceHeader;

    int clockRate{1000};
    /*读取ts文件用的一些参数*/
    uint8_t transportStreamBuffer[TRANSPORT_STREAM_PACKETS_SIZE];
    DemuxPacket demux;
    uint32_t currentPacket{0};


    std::ifstream fs;


private:
    NALPicture *unoccupiedPicture{nullptr};
//    bool finishFlag{false};
    NALHeader nalUnitHeader;
    NALDecodedPictureBuffer gop;

    /*计算pts和dts用的*/
    uint64_t videoDecodeFrameNumber{0};
    uint64_t videoDecodeIdrFrameNumber{0};

    NALSeqParameterSet spsList[32];
    NALPictureParameterSet ppsList[256];

public:

    void putData(uint8_t *data, uint32_t size);

    int getVideoParameter();


    int init1(const std::string &dir, uint32_t transportStreamPacketNumber, int timestamp);

    int init2();

    int readNalUint(uint8_t *&data, uint32_t &size);

    int getVideoFrame1(NALPicture *&picture);

    int getVideoFrame2(NALPicture *&picture, uint8_t *data, uint32_t size, uint8_t startCodeLength);

    int getVideoFrame3(NALPicture *&picture);


    NALPicture *allocPicture();

    void reset();

    void resetBuffer();

    ~NALReader();

private:


    int test1(NALPicture *picture, uint8_t *data, uint32_t size, uint8_t startCodeLength);

    int findNALU(uint8_t *&pos1, uint8_t *&pos2, int &startCodeLen1, int &startCodeLen2);

    int getTransportStreamData();


    void computedTimestamp(NALPicture *picture);
};


#endif //MUX_NALREADER_H
