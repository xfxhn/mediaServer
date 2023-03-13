#include <cstring>
#include "writeStream.h"

WriteStream::WriteStream(uint8_t *buf, uint32_t size) {

    memset(buf, 0, size);
    bufferStart = buf;
    currentPtr = buf;
    endPtr = buf + size - 1;
    bufferSize = size;
    position = 0;

}

uint8_t WriteStream::writeBit(uint8_t bit) {
    --bitsLeft;
    uint8_t val = *currentPtr >> (bitsLeft);
    *currentPtr = (unsigned) (val | bit) << (bitsLeft);

    if (bitsLeft == 0) {
        currentPtr++;
        position++;
        bitsLeft = 8;
    }
    return 0;
}

int WriteStream::writeMultiBit(uint32_t n, uint64_t val) {
    uint8_t bitPos = n;
    for (int i = 0; i < n; ++i) {
        --bitPos;
        uint8_t bit = (val >> bitPos) & 1u;
        writeBit(bit);
    }
    return 0;
}

int WriteStream::setString(const char *str, uint32_t n) {
    for (int i = 0; i < n; ++i) {
        writeMultiBit(8, str[i]);
    }
    return 0;
}

void WriteStream::setBytePtr(uint32_t n) {
    currentPtr += n;
    position += n;
    bitsLeft = 8;
}

void WriteStream::paddingByte(uint32_t n, uint8_t val) {
    for (int i = 0; i < n; ++i) {
        *currentPtr++ = val;
        ++position;
    }
    bitsLeft = 8;
}

void WriteStream::fillByte(uint8_t val) const {
    memset(currentPtr, val, bufferSize - position);
}

/*初始化为最先的样子*/
void WriteStream::reset() {
    memset(bufferStart, 0, bufferSize);
    currentPtr = bufferStart;
    position = 0;
    bitsLeft = 8;
}


