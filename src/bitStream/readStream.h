
#ifndef DEMUX_READSTREAM_H
#define DEMUX_READSTREAM_H

#include "cstdint"

class ReadStream {
private:

    //指向开始的位置
    uint8_t *startPtr = nullptr;
    //指向结束的位置
    uint8_t *endPtr = nullptr;
    // 当前读取到了字节中的第几位
    uint8_t bitsLeft{8};

public:

    // buffer 的长度（单位 Byte）
    uint32_t size{0};
    // 读取到第几字节
    uint32_t position{0};


    // 当前读取到了哪个字节的指针
    uint8_t *currentPtr = nullptr;

    ReadStream(uint8_t *buf, uint32_t _size);

    //读取1bit
    uint8_t readBit();

    //读取n个bit
    uint64_t readMultiBit(uint32_t n);

    /*无符号指数哥伦布*/
    uint32_t readUE();

    /*有符号指数哥伦布*/
    int32_t readSE();

    int getString(char str[], uint32_t n);

    //获取n个bit
    uint64_t getMultiBit(uint32_t n);

    /*字节对齐*/
    int byteAlignment();

    /*在当前位置往后跳几个字节*/
    int setBytePtr(uint32_t n);

    /*返回还剩多少个bit未解码*/
    uint32_t bitsToDecode() const;


    int more_rbsp_data();
};

#endif //DEMUX_READSTREAM_H
