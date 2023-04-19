

#ifndef RTSP_HTTP_H
#define RTSP_HTTP_H

#include <string>
#include <filesystem>
#include <thread>
#include "httpHls.h"
#include "httpFlv.h"
#include "socket/TcpSocket.h"

class Http {
private:
    HttpHls hls;
    HttpFlv flv;

    SOCKET clientSocket{0};
    std::map<std::string, std::string> request;
    static uint8_t response[1024];
    std::thread *sendFlVThread{nullptr};

public:
    bool stopFlag{true};

    int init(SOCKET socket);

    int parse(std::string &packet, const std::string &data);

    ~Http();

private:

    int sendFLV(std::filesystem::path path);

    int responseData(int status, const std::string &msg) const;
};


#endif //RTSP_HTTP_H
