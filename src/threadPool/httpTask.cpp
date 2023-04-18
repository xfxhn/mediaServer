#include "httpTask.h"
#include "protocol/http.h"


HttpTask::HttpTask(SOCKET socket) : clientSocket(socket) {
}

int HttpTask::run() {

    int ret;

    Http http;
    http.init(clientSocket);


    std::string packet;
    char buffer[TcpSocket::MAX_BUFFER]{0};
    int length = 0;

    while (http.stopFlag) {

        ret = TcpSocket::receive(clientSocket, buffer, length);
        if (ret < 0) {
            fprintf(stderr, "http.receive failed\n");
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
            ret = http.parse(packet, data);
            if (ret < 0) {
                fprintf(stderr, "http.parseRtsp失败\n");
                return ret;
            }

            // 继续查找下一个分隔符位置
            pos = packet.find("\r\n\r\n");
        }
    }

    return 0;
}

HttpTask::~HttpTask() {
    /*退出的时候关闭这个连接*/
    TcpSocket::closeSocket(clientSocket);
}
