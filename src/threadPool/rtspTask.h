
#ifndef RTSP_RTSPTASK_H
#define RTSP_RTSPTASK_H


#include "task.h"
#include "protocol/rtsp.h"
#include "socket/TcpSocket.h"


class RtspTask : public Task {

private:
    /* TcpSocket &rtsp;*/
    SOCKET clientSocket{};


public:
    explicit RtspTask(SOCKET socket) : clientSocket(socket) {

    }

    /*多个不同请求   比如有rtsp 和 http*/
    int run() override;

    ~RtspTask() override {
        /*退出的时候关闭这个连接*/
        TcpSocket::closeSocket(clientSocket);
    }

};


#endif //RTSP_RTSPTASK_H
