
#ifndef MEDIASERVER_AVWRITEPACKET_H
#define MEDIASERVER_AVWRITEPACKET_H

#include <cstdint>
#include <cstring>
#include <string>
#include "nalu/NALReader.h"
#include "adts/adtsReader.h"
#include "transportStream/transportPacket.h"

struct TransportStreamInfo {
    std::string name;
    double duration;
};

class AVWritePacket {
private:
    std::string path;
    uint32_t currentPacket;
    TransportPacket ts;
    std::ofstream transportStreamFileSystem;
    std::ofstream m3u8FileSystem;


    NALReader videoReader;
    NALPicture *picture;
    AdtsReader audioReader;
    AdtsHeader header;


    uint32_t seq{0};
    double lastDuration{0};


    std::vector<TransportStreamInfo> list;
public:
    int init(const std::string &dir, uint32_t transportStreamPacketNumber);

    int setVideoParameter(uint8_t *data, uint32_t size);

    int setAudioParameter(uint8_t audioObjectType, uint8_t samplingFrequencyIndex, uint8_t channelConfiguration);

    int writeFrame(uint8_t *data, uint32_t size, const std::string &type, bool flag);

private:
    int writeTransportStream();
};


#endif //MEDIASERVER_AVWRITEPACKET_H
