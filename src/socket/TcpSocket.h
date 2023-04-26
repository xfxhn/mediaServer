

#ifndef RTSP_TCPSOCKET_H
#define RTSP_TCPSOCKET_H

#include <cstdint>

#include "Socket.h"

class TcpSocket {
public:
    SOCKET serverSocket{0};
    /*SOCKET clientSocket{0};*/
    char clientIp[40]{0};
    int clientPort{0};
    static constexpr int MAX_BUFFER = 1024;
public:
    /*创建tcp套接字*/
    int createSocket();

    int bindSocket(const char *ip, int port) const;

    int listenPort(int backlog = 10) const;

    ~TcpSocket();

public:
    SOCKET acceptClient();

    static int receive(SOCKET clientSocket, char *data, int &dataSize);

    static int sendData(SOCKET clientSocket, uint8_t *buffer, int length);

    static int closeSocket(SOCKET clientSocket);

};


#endif //RTSP_TCPSOCKET_H
