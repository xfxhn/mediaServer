
#ifndef MUX_ADTSREADER_H
#define MUX_ADTSREADER_H

#include <fstream>
#include "adtsHeader.h"
#include "demuxPacket.h"

#define TRANSPORT_STREAM_PACKETS_SIZE 188

class AdtsHeader;


class AdtsReader {
public:
    static constexpr uint32_t MAX_BUFFER_SIZE{8191};
    static constexpr uint32_t MAX_HEADER_SIZE{7};

    /*给外部提供参数信息用的*/
    AdtsHeader parameter;
private:
    bool resetFlag{true};
    uint32_t currentPacket{0};
    std::string path;

    DemuxPacket demux;
    uint8_t transportStreamBuffer[TRANSPORT_STREAM_PACKETS_SIZE];


    std::ifstream fs;
    uint8_t *buffer{nullptr};
    uint8_t *bufferEnd{nullptr};
    uint8_t *bufferPosition{nullptr};


    uint32_t blockBufferSize{0};

    uint64_t audioDecodeFrameNumber{0};
public:
//    int init(const char *filename);

    int getAudioParameter();

    int init1(const std::string &dir, uint32_t transportStreamPacketNumber);

    int init2();

    int findFrame(AdtsHeader &header);

    void reset();

    int getAudioFrame1(AdtsHeader &header);

    int getAudioFrame2(AdtsHeader &header);

    void resetBuffer();

    void disposeAudio(AdtsHeader &header, uint8_t *data, uint32_t size);

    void putData(uint8_t *data, uint32_t size);

    ~AdtsReader();

private:
    int getTransportStreamData();

//    int adts_sequence(AdtsHeader &header, bool &stopFlag);
//
//    int fillBuffer();
//
//    int advanceBuffer(uint16_t frameLength);

};


#endif //MUX_ADTSREADER_H
