
#ifndef MEDIASERVER_HTTPHLS_H
#define MEDIASERVER_HTTPHLS_H

#include <string>
#include <filesystem>
#include "TcpSocket.h"

class HttpHls {
private:
    std::string mimeType;
    std::string path;
    SOCKET clientSocket;
public:
    int init(std::filesystem::path &path, SOCKET socket);

    int disposeHls();
};


#endif //MEDIASERVER_HTTPHLS_H
