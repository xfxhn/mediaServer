
#ifndef MEDIASERVER_HTTPFLV_H
#define MEDIASERVER_HTTPFLV_H

#include <string>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include "socket/TcpSocket.h"
#include "AVPacket/AVReadPacket.h"

#include "flashVideo/FLVTagHeader.h"
#include "flashVideo/FLVAudioTag.h"
#include "flashVideo/FLVVideoTag.h"

class HttpFlv {
private:
    FLVTagHeader tagHeader;
    FLVAudioTag audioTag;
    FLVVideoTag videoTag;

    uint8_t response[2048];
    AVPackage *package{nullptr};
    int transportStreamPacketNumber{0};
    std::string dir;
    SOCKET clientSocket;


    uint32_t previousTagSize{0};

    bool stopFlag{true};
public:
    void setStopFlag(bool flag);

    int init(std::filesystem::path &path, SOCKET socket);

    int disposeFlv();

    ~HttpFlv();

private:
    int sendHeader();

    int sendMetadata();

    int sendSequenceHeader();

    int sendVideoSequenceHeader(AVReadPacket &packet);

    int sendAudioSequenceHeader(AVReadPacket &packet);

    int sendData();

    int sendAudioData();

    int sendVideoData();
};


#endif //MEDIASERVER_HTTPFLV_H
