
#ifndef MEDIASERVER_HTTPFLV_H
#define MEDIASERVER_HTTPFLV_H

#include <string>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include "TcpSocket.h"
#include "AVPacket.h"

class HttpFlv {
private:


    AVPackage *package{nullptr};
    int transportStreamPacketNumber{0};
    std::string dir;
    SOCKET clientSocket;


    uint8_t *audioBuffer{nullptr};
    uint8_t *videoBuffer{nullptr};

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
