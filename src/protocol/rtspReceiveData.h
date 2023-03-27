//
// Created by Administrator on 2023-03-27.
//

#ifndef RTSP_RTSPRECEIVEDATA_H
#define RTSP_RTSPRECEIVEDATA_H

#include "transportPacket.h"

class RtspReceiveData {
private:
    TransportPacket ts;
    uint8_t *buffer{nullptr};
};


#endif //RTSP_RTSPRECEIVEDATA_H
