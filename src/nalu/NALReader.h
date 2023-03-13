
#ifndef MUX_NALREADER_H
#define MUX_NALREADER_H

#include <cstdint>
#include <fstream>
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
    bool pictureFinishFlag{false};
    NALSeqParameterSet sps;
    NALPictureParameterSet pps;
    NALSliceHeader sliceHeader;
    uint8_t *spsData{nullptr};
    uint8_t *ppsData{nullptr};
    uint8_t spsSize{0};
    uint8_t ppsSize{0};



private:

    std::ifstream fs;
    uint8_t *bufferStart{nullptr};
    uint8_t *bufferPosition{nullptr};
    uint8_t *bufferEnd{nullptr};
    uint32_t blockBufferSize{0};

private:

    bool finishFlag{false};
    NALHeader nalUnitHeader;
    NALPicture *unoccupiedPicture{nullptr};
    NALDecodedPictureBuffer gop;

    uint64_t videoDecodeFrameNumber{0};
    uint64_t videoDecodeIdrFrameNumber{0};

    NALSeqParameterSet spsList[32];
    NALPictureParameterSet ppsList[256];


private:
    static int getNextStartCode(uint8_t *bufPtr, const uint8_t *end, uint8_t *&pos1, uint8_t *&pos2, int &startCodeLen1,
                                int &startCodeLen2);

    void computedTimestamp(NALPicture *picture);

public:
    int init(const char *filename);

    int getSpsAndPps(uint32_t &spsSize, uint32_t &ppsSize);

    int readNalUint(uint8_t *&data, uint32_t &size, int &startCodeLength, bool &isStopLoop);

    int getVideoFrame(NALPicture *&picture, bool &flag);

    int test(NALPicture *&picture, bool &flag);

    int putNalUintData(NALPicture *&picture, uint8_t *data, uint32_t size);


    int test1(NALPicture *&picture, uint8_t *data, uint32_t size);

    NALPicture *allocPicture();

    void reset();


    ~NALReader();
};


#endif //MUX_NALREADER_H
