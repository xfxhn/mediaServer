
#ifndef DEMUX_WRITESTREAM_H
#define DEMUX_WRITESTREAM_H

#include <cstdint>

class WriteStream {
private:
    uint8_t *endPtr = nullptr;
    // 当前读取到了字节中的第几位
    uint8_t bitsLeft = 8;


public:
    //指向开始的位置
    uint8_t *bufferStart = nullptr;
    // 写到第几字节
    uint32_t position;
    // 当前读取到了哪个字节的指针
    uint8_t *currentPtr = nullptr;
    // buffer 的长度（单位 Byte）
    uint32_t bufferSize = 0;
public:
    WriteStream(uint8_t *buf, uint32_t _size);

    //写入1bit
    uint8_t writeBit(uint8_t bit);

    //写入n个bit
    int writeMultiBit(uint32_t n, uint64_t val);

    void fillByte(uint8_t val) const;

    void paddingByte(uint32_t n, uint8_t val);

    int setString(const char str[], uint32_t n);

    void setBytePtr(uint32_t n);

    void reset();
};

#endif //DEMUX_WRITESTREAM_H
