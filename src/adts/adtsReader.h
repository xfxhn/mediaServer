
#ifndef MUX_ADTSREADER_H
#define MUX_ADTSREADER_H

#include <fstream>
#include "adtsHeader.h"


class AdtsReader {
public:
    static constexpr uint32_t MAX_BUFFER_SIZE{8191};
    static constexpr uint32_t MAX_HEADER_SIZE{7};

    /*给外部提供参数信息用的*/
    AdtsHeader parameter;
private:
    bool resetFlag{true};

    uint8_t *buffer{nullptr};
    uint8_t *bufferEnd{nullptr};
    uint8_t *bufferPosition{nullptr};

    uint32_t blockBufferSize{0};

    uint64_t audioDecodeFrameNumber{0};
public:

    int init();

    int getParameter();

    int getAudioFrame2(AdtsHeader &header);

    void resetBuffer();

    void disposeAudio(AdtsHeader &header, uint8_t *data, uint32_t size);

    void putData(uint8_t *data, uint32_t size);

    ~AdtsReader();


};


#endif //MUX_ADTSREADER_H
