
#ifndef RTSP_RTSP_H
#define RTSP_RTSP_H

#include <cstdint>
#include <thread>
#include <string>
#include <vector>
#include <map>
#include "TcpSocket.h"


struct SdpInfo {
    std::string version;
    std::map<std::string, std::string> origin;
    std::string name;
    std::map<std::string, std::string> timing;
    std::map<std::string, std::string> media[2];
    uint8_t spsData[50];
    uint8_t spsSize{0};
    uint8_t ppsData[50];
    uint8_t ppsSize{0};

    uint8_t audioObjectType{0};
    uint8_t samplingFrequencyIndex{0};
    uint8_t channelConfiguration{0};
};


/*struct Info {
    std::string session;
    std::string dir;
    SdpInfo sdp;
    *//*这个流写入到第几个ts包*//*
    int transportStreamPacketNumber{0};
};*/

class ReadStream;

class Rtsp {

public:
    bool stopFlag{true};

public:
    int init(SOCKET socket);

    int parseRtsp(std::string &packet, const std::string &data);

    ~Rtsp();

private:
    SdpInfo info;
    int transportStreamPacketNumber{-1};
    static char response[2048];
    std::map<std::string, std::string> obj;
    SOCKET clientSocket;

    std::thread *videoSendThread{nullptr};
    std::thread *audioSendThread{nullptr};

    uint8_t videoChannel{0};
    uint8_t audioChannel{0};

    std::string videoControl;
    std::string audioControl;

    /*这个是当前rtsp流生成的唯一id*/
    std::string uniqueSession;

    bool stopVideoSendFlag{true};
    bool stopAudioSendFlag{true};

    /*拉流用的*/
    std::string dir;
    bool flag{false};
private:

    int receiveData(std::string &packet);


    int sendVideo(uint32_t number);


    int sendAudio(uint32_t number);

    static int parseSdp(const std::string &sdp, SdpInfo &sdpInfo);

    static int parseMediaLevel(int i, const std::vector<std::string> &list, SdpInfo &sdpInfo);

    static int parseAACConfig(const std::string &config, SdpInfo &sdpInfo);


    int responseData(int status, const std::string &msg);

};


#endif //RTSP_RTSP_H
