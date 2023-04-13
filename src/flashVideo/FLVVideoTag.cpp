
#include "FLVVideoTag.h"
#include <string>
#include <cstring>

int FLVVideoTag::writeData(WriteStream &ws) {
    ws.writeMultiBit(4, frameType);
    ws.writeMultiBit(4, codecID);
    ws.writeMultiBit(8, AVCPacketType);
    ws.writeMultiBit(24, compositionTime);
    return 0;
}

int FLVVideoTag::writeConfig(WriteStream &ws, uint8_t *sps, uint8_t spsSize, uint8_t *pps, uint8_t ppsSize) {
    ws.writeMultiBit(4, frameType);
    ws.writeMultiBit(4, codecID);
    ws.writeMultiBit(8, AVCPacketType);
    ws.writeMultiBit(24, compositionTime);

    /*-----AVC config------*/
    ws.writeMultiBit(8, configurationVersion);
    ws.writeMultiBit(8, AVCProfileIndication);

    ws.writeMultiBit(8, profile_compatibility);

    ws.writeMultiBit(8, AVCLevelIndication);
    ws.writeMultiBit(6, 0);
    ws.writeMultiBit(2, lengthSizeMinusOne);

    ws.writeMultiBit(3, 0);
    /*--- write sps ---*/
    ws.writeMultiBit(5, numOfSequenceParameterSets);
    ws.writeMultiBit(16, spsSize);
    memcpy(ws.currentPtr, sps, spsSize);
    ws.setBytePtr(spsSize);
    /*--- write pps ---*/
    ws.writeMultiBit(8, numOfPictureParameterSets);
    ws.writeMultiBit(16, ppsSize);
    memcpy(ws.currentPtr, pps, ppsSize);
    ws.setBytePtr(ppsSize);
    return 0;
}

void FLVVideoTag::setConfig(uint8_t type, FRAME_TYPE packetType, uint32_t timestamp) {
    frameType = type;
    AVCPacketType = packetType;
    compositionTime = timestamp;
}

void FLVVideoTag::setConfigurationRecord(uint8_t profile, uint8_t level, uint8_t compatibility) {
    AVCProfileIndication = profile;
    AVCLevelIndication = level;
    profile_compatibility = compatibility;
}


