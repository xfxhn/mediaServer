
#ifndef MUX_ADTSREADER_H
#define MUX_ADTSREADER_H

#include <fstream>

class AdtsHeader;


class AdtsReader {
public:
    static constexpr uint32_t MAX_BUFFER_SIZE{8191};
    static constexpr uint32_t MAX_HEADER_SIZE{7};
private:
    uint64_t audioDecodeFrameNumber{0};

    std::ifstream fs;
    uint8_t *buffer{nullptr};


    uint32_t blockBufferSize{0};
    /*已经读取了多少字节*/
    uint32_t fillByteSize{0};
    bool isEof{true};
public:
    int init(const char *filename);

    void reset();

    int getAudioFrame(AdtsHeader &header, bool &flag);

    int putAACData(AdtsHeader &header, uint8_t *data, uint32_t size);

    ~AdtsReader();

private:
    int adts_sequence(AdtsHeader &header, bool &stopFlag);

    int fillBuffer();

    int advanceBuffer(uint16_t frameLength);

};


#endif //MUX_ADTSREADER_H
