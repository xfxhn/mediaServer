

#ifndef MUX_FLVHEADER_H
#define MUX_FLVHEADER_H

#include <cstdint>

class WriteStream;

class FLVHeader {
public:
    constexpr static int size{9};

    int write(WriteStream &ws);

private:
    char Signature[3]{'F', 'L', 'V'};
    uint8_t Version{1};
    //uint8_t TypeFlagsReserved{0};
    uint8_t TypeFlagsAudio{1};
    //uint8_t TypeFlagsReserved{0};
    uint8_t TypeFlagsVideo{1};
    /*这个header的长度*/
    uint32_t DataOffset{9};
};


#endif //MUX_FLVHEADER_H
