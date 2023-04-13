

#ifndef RTSP_HTTP_H
#define RTSP_HTTP_H

#include <string>
#include <map>
#include <filesystem>
#include "httpHls.h"
#include "httpFlv.h"
#include "TcpSocket.h"
#include "AVPacket.h"

class Http {
private:
    HttpHls hls;
    HttpFlv flv;

    AVPackage *package{nullptr};
    int transportStreamPacketNumber{0};
    std::string dir;
    std::map<std::string, std::string> request;
    static uint8_t response[1024];
    SOCKET clientSocket{0};
    uint32_t previousTagSize{0};


    uint8_t *audioBuffer{nullptr};
    uint8_t *videoBuffer{nullptr};
public:
    int init(SOCKET socket);

    int parse(std::string &packet, const std::string &data);

    ~Http();

private:
    int sendHeader(const std::string &mimeType);

    int sendMetadata();

    int sendSequenceHeader();

    int sendVideoSequenceHeader(AVPacket &packet);

    int sendAudioSequenceHeader(AVPacket &packet);

    int sendData();

    int sendAudioData();

    int sendVideoData();

    int disposeTransStream(const std::string &path, const std::string &mimeType);

    int initFLV(std::filesystem::path &path, const std::string &mimeType);

    int responseData(int status, const std::string &msg) const;
};


#endif //RTSP_HTTP_H
