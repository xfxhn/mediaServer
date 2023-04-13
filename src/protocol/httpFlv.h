
#ifndef MEDIASERVER_HTTPFLV_H
#define MEDIASERVER_HTTPFLV_H

#include <string>
#include <cstdint>
#include <filesystem>
#include "TcpSocket.h"

class HttpFlv {
private:
    std::string mimeType{"video/x-flv"};
    int transportStreamPacketNumber{0};
    std::string dir;
    SOCKET clientSocket;


    uint8_t *audioBuffer{nullptr};
    uint8_t *videoBuffer{nullptr};

    uint32_t previousTagSize{0};
public:
    int init(std::filesystem::path &path, SOCKET socket);

    int disposeFlv();

private:
    int sendHeader();

    int sendMetadata();
};


#endif //MEDIASERVER_HTTPFLV_H
