

#ifndef RTSP_HTTP_H
#define RTSP_HTTP_H

#include <string>
#include <filesystem>
#include "httpHls.h"
#include "httpFlv.h"
#include "TcpSocket.h"

class Http {
private:
    HttpHls hls;
    HttpFlv flv;

    SOCKET clientSocket{0};
    std::map<std::string, std::string> request;
    static uint8_t response[1024];



public:
    int init(SOCKET socket);

    int parse(std::string &packet, const std::string &data);

private:


    int responseData(int status, const std::string &msg) const;
};


#endif //RTSP_HTTP_H
