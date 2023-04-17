
#ifndef MUX_NALREADER_H
#define MUX_NALREADER_H

#include <cstdint>
#include <fstream>
#include <cstring>
#include "NALHeader.h"
#include "NALPicture.h"
#include "NALDecodedPictureBuffer.h"


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
    NALSeqParameterSet sps;
    NALPictureParameterSet pps;
    uint8_t spsData[50];
    uint8_t ppsData[50];
    uint8_t spsSize{0};
    uint8_t ppsSize{0};


    uint8_t *bufferStart{nullptr};
    uint8_t *bufferPosition{nullptr};
    uint8_t *bufferEnd{nullptr};
    uint32_t blockBufferSize{0};

private:
    bool resetFlag{true};
    NALPicture *unoccupiedPicture{nullptr};
    NALHeader nalUnitHeader;
    NALDecodedPictureBuffer gop;

    /*计算pts和dts用的*/
    uint32_t videoDecodeFrameNumber{0};
    uint32_t videoDecodeIdrFrameNumber{0};

    NALSeqParameterSet spsList[32];
    NALPictureParameterSet ppsList[256];

public:
    int init();

    void putData(uint8_t *data, uint32_t size);

    int getParameter();

    int getVideoFrame2(NALPicture *&picture, uint8_t *data, uint32_t size, uint8_t startCodeLength);

    int getVideoFrame3(NALPicture *&picture);


    NALPicture *allocPicture();

    void resetBuffer();

    ~NALReader();

private:

    int disposePicture(NALPicture *picture, uint8_t *data, uint32_t size, uint8_t startCodeLength);

    int findNALU(uint8_t *&pos1, uint8_t *&pos2, int &startCodeLen1, int &startCodeLen2) const;

    void computedTimestamp(NALPicture *picture);
};


#endif //MUX_NALREADER_H
