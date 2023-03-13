#ifndef DEMUX_AVCDECODERCONFIGURATIONRECORD_H
#define DEMUX_AVCDECODERCONFIGURATIONRECORD_H

#include <cstdint>

class ReadStream;

class WriteStream;

class NALPicture;

class AVCDecoderConfigurationRecord {
public:

    uint8_t configurationVersion{1};
    uint8_t AVCProfileIndication{0};
    uint8_t profile_compatibility{0};
    uint8_t AVCLevelIndication{0};

    uint8_t lengthSizeMinusOne{3};

    uint8_t numOfSequenceParameterSets{1};


    uint8_t numOfPictureParameterSets{1};


    uint8_t chroma_format{1};
    uint8_t bit_depth_luma_minus8{0};
    uint8_t bit_depth_chroma_minus8{0};

    uint8_t numOfSequenceParameterSetExt{0};
    uint16_t sequenceParameterSetExtLength[256]{0};
    uint8_t *sequenceParameterSetExtNALUnit[256]{nullptr};

public:
    int write(WriteStream &ws, const NALPicture *picture) const;

};

#endif //DEMUX_AVCDECODERCONFIGURATIONRECORD_H