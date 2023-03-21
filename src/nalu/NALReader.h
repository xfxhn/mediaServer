
#ifndef MUX_NALREADER_H
#define MUX_NALREADER_H

#include <cstdint>
#include <fstream>
#include <cstring>
#include "NALHeader.h"
#include "NALPicture.h"
#include "NALDecodedPictureBuffer.h"


#include "demuxPacket.h"

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
    bool pictureFinishFlag{false};
    NALSeqParameterSet sps;
    NALPictureParameterSet pps;
    NALSliceHeader sliceHeader;
    uint8_t spsData[50];
    uint8_t ppsData[50];
    uint8_t spsSize{0};
    uint8_t ppsSize{0};

private:
    uint8_t transportStreamBuffer[188];
    DemuxPacket demux;
    uint32_t currentPacket{0};
    std::ifstream fs;
    uint8_t *bufferStart{nullptr};
    uint8_t *bufferPosition{nullptr};
    uint8_t *bufferEnd{nullptr};
    uint32_t blockBufferSize{0};

    NALPicture *unoccupiedPicture{nullptr};
private:

    bool finishFlag{false};
    NALHeader nalUnitHeader;

    NALDecodedPictureBuffer gop;

    uint64_t videoDecodeFrameNumber{0};
    uint64_t videoDecodeIdrFrameNumber{0};

    NALSeqParameterSet spsList[32];
    NALPictureParameterSet ppsList[256];


public:


    int init(const char *filename);

    int init1(const std::string &dir, uint32_t transportStreamPacketNumber);

    int readNalUint1(uint8_t *&data, uint32_t &size);

    int readNalUint(uint8_t *&data, uint32_t &size, int &startCodeLength, bool &isStopLoop);

    int getVideoFrame(NALPicture *&picture, bool &flag);


    int test1(NALPicture *&picture, uint8_t *data, uint32_t size);

    NALPicture *allocPicture();

    void reset();


    ~NALReader();

private:


    int getNalUintData();

    static int getNextStartCode(uint8_t *bufPtr, const uint8_t *end, uint8_t *&pos1, uint8_t *&pos2, int &startCodeLen1,
                                int &startCodeLen2);

    void computedTimestamp(NALPicture *picture);
};


#endif //MUX_NALREADER_H
