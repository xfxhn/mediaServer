

#ifndef MEDIASERVER_FLVTAGHEADER_H
#define MEDIASERVER_FLVTAGHEADER_H

#include "bitStream/writeStream.h"


enum {
    AUDIO = 8,
    VIDEO = 9,
    SCRIPT = 18
};

class FLVTagHeader {
public:
    void setConfig(uint8_t type, uint32_t size, uint64_t dts);

    int write(WriteStream &ws) const;

private:
    /*是否加密*/
    uint8_t Filter{0};
    /*此tag中的内容类型*/
    uint8_t TagType{0};
    /*表明当前tag中存储的音频或视频文件的大小，包括文件的头*/
    /*消息的长度。从StreamID到标签结尾的字节数(等于标签的长度- 11)*/
    uint32_t DataSize{0};

    uint32_t Timestamp{0};
    uint8_t TimestampExtended{0};

    uint32_t StreamID{0};
};


#endif //MEDIASERVER_FLVTAGHEADER_H
