
#ifndef RTSP_UDPSOCKET_H
#define RTSP_UDPSOCKET_H

#include "Socket.h"

class UdpSocket {
public:
    SOCKET sock{0};
public:
    int createSocket();

    int bindSocket(const char *ip, int port) const;
};


#endif //RTSP_UDPSOCKET_H
