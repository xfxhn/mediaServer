
#ifndef MEDIASERVER_HTTPFLV_H
#define MEDIASERVER_HTTPFLV_H

#include <string>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include "socket/TcpSocket.h"
#include "AVPacket/AVPacket.h"

class HttpFlv {
private:

    uint8_t response[2048];


    AVPackage *package{nullptr};
    int transportStreamPacketNumber{0};
    std::string dir;
    SOCKET clientSocket;


    uint32_t previousTagSize{0};
public:
    int init(std::filesystem::path &path, SOCKET socket);

    int disposeFlv();

    ~HttpFlv();

private:
    int sendHeader();

    int sendMetadata();

    int sendSequenceHeader();

    int sendVideoSequenceHeader(AVPacket &packet);

    int sendAudioSequenceHeader(AVPacket &packet);

    int sendData();

    int sendAudioData();

    int sendVideoData();
};


#endif //MEDIASERVER_HTTPFLV_H
