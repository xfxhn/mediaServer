

#ifndef MEDIASERVER_FLVVIDEOTAG_H
#define MEDIASERVER_FLVVIDEOTAG_H

#include <cstdint>
#include <vector>
#include "bitStream/writeStream.h"

enum FRAME_TYPE {
    AVC_sequence_header = 0,
    AVC_NALU = 1,
    AVC_end_of_sequence = 2
};

/*enum CODEC_ID {
    AVC_standard = 7
};*/

class FLVVideoTag {
private:
    uint8_t frameType{0};
    /*编码标准*/
    uint8_t codecID{7};
    FRAME_TYPE AVCPacketType{AVC_end_of_sequence};
    uint32_t compositionTime{0};
private:
    uint8_t configurationVersion{1};
    uint8_t AVCProfileIndication{0};
    uint8_t profile_compatibility{0};
    uint8_t AVCLevelIndication{0};
    /*这个字段表示用几字节存储nalu的长度，这里设置为3的话，表示使用4字节存储 3+1*/
    uint8_t lengthSizeMinusOne{3};
    uint8_t numOfSequenceParameterSets{1};
    uint8_t numOfPictureParameterSets{1};
public:
    void setConfig(uint8_t type, FRAME_TYPE packetType, uint32_t timestamp);

    void setConfigurationRecord(uint8_t profile_idc, uint8_t level_idc, uint8_t compatibility);

    int writeConfig(WriteStream &ws, uint8_t *sps, uint8_t spsSize, uint8_t *pps, uint8_t ppsSize);

    int writeData(WriteStream &ws);
};


#endif //MEDIASERVER_FLVVIDEOTAG_H
