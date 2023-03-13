﻿
#ifndef RTSP_UDPSOCKET_H
#define RTSP_UDPSOCKET_H

#ifdef _WIN32

#include <winsock.h>

#else
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

class UdpSocket {
public:
    SOCKET sock{0};
public:
    int createSocket();

    int bindSocket(const char *ip, int port) const;
};


#endif //RTSP_UDPSOCKET_H
