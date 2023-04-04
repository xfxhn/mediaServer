

#ifndef RTSP_HTTP_H
#define RTSP_HTTP_H

#include <string>
#include "TcpSocket.h"

class Http {
private:
    SOCKET clientSocket;
public:
    int init(SOCKET socket);

    int parse(std::string &packet, const std::string &data);

    ~Http();

private:
    static inline std::string getBody(const std::string &str);

};


#endif //RTSP_HTTP_H
