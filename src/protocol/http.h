

#ifndef RTSP_HTTP_H
#define RTSP_HTTP_H

#include <string>
#include "TcpSocket.h"

class Http {
private:
    static char response[1024];
    SOCKET clientSocket{0};
public:
    int init(SOCKET socket);

    int parse(std::string &packet, const std::string &data);

    ~Http();

private:

    int responseData(int status, const std::string &msg) const;
};


#endif //RTSP_HTTP_H
