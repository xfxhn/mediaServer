
#include "AVCDecoderConfigurationRecord.h"

#include <cstring>
#include <cstdio>

#include "writeStream.h"
#include "readStream.h"
#include "NALPicture.h"


int AVCDecoderConfigurationRecord::write(WriteStream &ws, const NALPicture *picture) const {
    int size = 0;
    const NALSeqParameterSet &sps = picture->sliceHeader.sps;

    ws.writeMultiBit(8, configurationVersion);
    ws.writeMultiBit(8, AVCProfileIndication);

    //ws.writeMultiBit(8, profile_compatibility);
    ws.writeMultiBit(1, sps.constraint_set0_flag);
    ws.writeMultiBit(1, sps.constraint_set1_flag);
    ws.writeMultiBit(1, sps.constraint_set2_flag);
    ws.writeMultiBit(1, sps.constraint_set3_flag);
    ws.writeMultiBit(1, sps.constraint_set4_flag);
    ws.writeMultiBit(1, sps.constraint_set5_flag);
    /*保留*/
    ws.writeMultiBit(2, 0);

    ws.writeMultiBit(8, AVCLevelIndication);
    size += 4;
    /*保留*/
    ws.writeMultiBit(6, 0);
    ws.writeMultiBit(2, lengthSizeMinusOne);
    size += 1;
    /*保留*/
    ws.writeMultiBit(3, 0);


    for (const auto &i: picture->data) {
        if (i.type == 1) {
            ws.writeMultiBit(5, numOfSequenceParameterSets);
            ws.writeMultiBit(16, i.nalUintSize);
            memcpy(ws.currentPtr, i.data, i.nalUintSize);
            ws.setBytePtr(i.nalUintSize);
        } else if (i.type == 2) {
            ws.writeMultiBit(8, numOfPictureParameterSets);
            ws.writeMultiBit(16, i.nalUintSize);
            memcpy(ws.currentPtr, i.data, i.nalUintSize);
            ws.setBytePtr(i.nalUintSize);
        } else {
            fprintf(stderr, "除了sps和pps还有其他数据？\n");
            return -1;
        }
    }
    size += 6;

    return size;
}


