
#ifndef RTSP_RTSPTASK_H
#define RTSP_RTSPTASK_H


#include "task.h"
#include "rtsp.h"
#include "TcpSocket.h"


class RtspTask : public Task {

private:
    /* TcpSocket &rtsp;*/
    SOCKET clientSocket;


public:
    explicit RtspTask(SOCKET socket) : clientSocket(socket) {

    }


    /*多个不同请求   比如有rtsp 和 http*/
    int run() override {
        int ret;

        Rtsp rtsp;
        ret = rtsp.init(clientSocket);
        if (ret < 0) {
            return ret;
        }

        char buffer[TcpSocket::MAX_BUFFER]{0};
        int length = 0;
        std::string packet;

        while (rtsp.stopFlag) {
            /*length本次获取的数据大小*/
            ret = TcpSocket::receive(clientSocket, buffer, length);
            if (ret < 0) {
                fprintf(stderr, "rtsp.receive failed\n");
                return ret;
            }


            // 将缓冲区中的数据添加到字符串对象中
            packet.append(buffer, length);
            // 查找分隔符在字符串中的位置
            std::string::size_type pos = packet.find("\r\n\r\n");
            while (pos != std::string::npos) {
                // 截取出一个完整的数据包（不含分隔符）
                std::string data = packet.substr(0, pos);
                // 去掉已处理的部分（含分隔符）
                packet.erase(0, pos + 4);

                /*处理请求，接收客户端发过来的数据*/
                ret = rtsp.parseRtsp(packet, data);
                if (ret < 0) {
                    fprintf(stderr, "rtsp.parseRtsp失败\n");
                    return ret;
                }


                // 继续查找下一个分隔符位置
                pos = packet.find("\r\n\r\n");
            }
        }

        return 0;


    }

    ~RtspTask() override {
        /*退出的时候关闭这个连接*/
        TcpSocket::closeSocket(clientSocket);
    }

};


#endif //RTSP_RTSPTASK_H
