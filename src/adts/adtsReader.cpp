#include "adtsReader.h"

#include <cstdio>
#include <cstring>
#include "util.h"
#include "readStream.h"
#include "adtsHeader.h"


#define min(a, b) ( (a) < (b) ? (a) : (b) )


int AdtsReader::init(const char *filename) {
    fs.open(filename, std::ios::out | std::ios::binary);
    if (!fs.is_open()) {
        fprintf(stderr, "open %s failed\n", filename);
        return -1;
    }


    //最大ADTS头是9，没有差错校验就是7
    buffer = new uint8_t[MAX_BUFFER_SIZE];
    fs.read(reinterpret_cast<char *>(buffer), MAX_BUFFER_SIZE);
    blockBufferSize = fs.gcount();


    if (blockBufferSize < MAX_HEADER_SIZE) {
        fprintf(stderr, "数据不完整\n");
        return -1;
    }

    if (memcmp(buffer, "ID3", 3) == 0) {
        fprintf(stderr, "不支持解析ID3\n");
        return -1;
    } else if (memcmp(buffer, "ADIF", 4) == 0) {
        fprintf(stderr, "不支持解析ADIF格式\n");
        return -1;
    }
    return 0;
}

int AdtsReader::init1(const std::string &dir, uint32_t transportStreamPacketNumber) {

    currentPacket = transportStreamPacketNumber;
    std::string name = "test" + std::to_string(currentPacket) + ".ts";
    fs.open("test/" + name, std::ios::out | std::ios::binary);
    if (!fs.is_open()) {
        fprintf(stderr, "open %s failed\n", name.c_str());
        return -1;
    }

    buffer = new uint8_t[MAX_BUFFER_SIZE];

    blockBufferSize = 0;

    return 0;
}

void AdtsReader::reset() {
    fs.seekg(0);
    fs.read(reinterpret_cast<char *>(buffer), MAX_BUFFER_SIZE);
    blockBufferSize = fs.gcount();
    fillByteSize = 0;
}

int AdtsReader::findFrame(AdtsHeader &header) {
    int ret;
    if (buffer == nullptr) {
        fprintf(stderr, "请初始化\n");
        return -1;
    }
    /*还剩多少字节未读取*/
    uint32_t remainingByte = bufferEnd - bufferPosition;

    memcpy(buffer, bufferPosition, remainingByte);
    blockBufferSize = remainingByte;
    bufferEnd = buffer + remainingByte;
    if (remainingByte < MAX_HEADER_SIZE) {
        ret = getTransportStreamData();
        if (ret < 0) {
            fprintf(stderr, "获取ts数据失败\n");
            return ret;
        }
    }

    while (true) {
        ReadStream rs(buffer, blockBufferSize);
        if (rs.getMultiBit(12) != 0xFFF) {
            fprintf(stderr, "格式不对,不等于0xFFF\n");
            return -1;
        }

        ret = header.adts_fixed_header(rs);
        if (ret < 0) {
            fprintf(stderr, "解析adts fixed header失败\n");
            return -1;
        }
        header.adts_variable_header(rs);

        uint16_t frameLength = header.frame_length;

        if (blockBufferSize < frameLength) {
            ret = getTransportStreamData();
            if (ret < 0) {
                fprintf(stderr, "获取ts数据失败\n");
                return ret;
            }
        } else {
            header.data = &buffer[MAX_HEADER_SIZE];
            header.size = frameLength - MAX_HEADER_SIZE;
            bufferPosition = buffer + frameLength;
            break;
        }
    }


    return 0;
}

int AdtsReader::adts_sequence(AdtsHeader &header, bool &stopFlag) {
    int ret;
    if (!buffer) {
        fprintf(stderr, "请初始化\n");
        return -1;
    }

    fillBuffer();
    if (blockBufferSize > MAX_HEADER_SIZE) {
        ReadStream bs(buffer, MAX_BUFFER_SIZE);
        if (bs.getMultiBit(12) != 0xFFF) {
            stopFlag = false;
            fprintf(stderr, "格式不对,不等于0xFFF\n");
            return -1;
        }
        /*读取每一帧的ADTS头*/

        ret = header.adts_fixed_header(bs);
        if (ret < 0) {
            stopFlag = false;
            fprintf(stderr, "解析adts fixed header失败\n");
            return -1;
        }
        header.adts_variable_header(bs);

        uint16_t frameLength = header.frame_length;

        /*如果这一帧的长度等于0或者大于filesize的话就退出，数据不对*/
        if (frameLength == 0 || frameLength > blockBufferSize) {
            stopFlag = false;
            fprintf(stderr, "AAC data 数据不对\n");
            return -1;
        }
        header.data = &buffer[MAX_HEADER_SIZE];
        header.size = frameLength - MAX_HEADER_SIZE;

        advanceBuffer(frameLength);
    } else {
        fillByteSize = 0;
        stopFlag = false;
    }

    return 0;
}

int AdtsReader::fillBuffer() {
    if (fillByteSize > 0) {
        if (blockBufferSize) {
            //由src所指内存区域复制count个字节到dest所指内存区域。
            memmove(buffer, (buffer + fillByteSize), blockBufferSize);
        }

        if (isEof) {
            fs.read(reinterpret_cast<char *>(buffer + blockBufferSize), fillByteSize);
            uint32_t size = fs.gcount();

            /*如果读不满 fillByteSize 大小代表读到尾部了size是实际读取了多大*/
            if (size != fillByteSize) {
                isEof = false;
            }
            blockBufferSize += size;
        }
        fillByteSize = 0;
    }
    return 0;
}

int AdtsReader::advanceBuffer(uint16_t length) {
    if (length > 0 && blockBufferSize > 0) {
        uint32_t size = min(length, blockBufferSize);
        fillByteSize += size;
        blockBufferSize -= size;
    }
    return 0;
}

int AdtsReader::getAudioFrame(AdtsHeader &header, bool &flag) {
    int ret;
    ret = adts_sequence(header, flag);
    if (ret < 0) {
        fprintf(stderr, "解析adts_sequence 失败\n");
        return ret;
    }
    header.duration += (1024.0 / header.sample_rate);
    header.dts = av_rescale_q(audioDecodeFrameNumber, {1, static_cast<int>(header.sample_rate)}, {1, 1000});
    header.pts = header.dts;
    audioDecodeFrameNumber += 1024;
    return 0;
}

int AdtsReader::getAudioFrame1(AdtsHeader &header) {
    int ret;
    ret = findFrame(header);
    if (ret < 0) {
        fprintf(stderr, "findFrame失败\n");
        return ret;
    }
    disposeAudio(header, header.data, header.size);
    return 0;
}

AdtsReader::~AdtsReader() {
    if (buffer) {
        delete[] buffer;
        buffer = nullptr;
    }
    fs.close();
}

void AdtsReader::disposeAudio(AdtsHeader &header, uint8_t *data, uint32_t size) {
    header.data = data;
    header.size = size;
    header.duration = (double) audioDecodeFrameNumber / header.sample_rate;
    header.dts = audioDecodeFrameNumber;
    header.pts = av_rescale_q(audioDecodeFrameNumber, {1, static_cast<int>(header.sample_rate)}, {1, 1000});
    header.interval = (int) (1024.0 / (double) header.sample_rate * 1000.0);
    audioDecodeFrameNumber += 1024;
}

int AdtsReader::getTransportStreamData() {
    int ret;

    while (true) {
        fs.read(reinterpret_cast<char *>(transportStreamBuffer), TRANSPORT_STREAM_PACKETS_SIZE);
        uint32_t size = fs.gcount();
        // bufferPosition += size;
        if (size != TRANSPORT_STREAM_PACKETS_SIZE) {
            /*表示这个文件读完了，读下一个*/
            /*这里ts文件，应该就是188的倍数，不是188的倍数，这个文件是有问题*/
            fs.close();
            std::string name = "test" + std::to_string(++currentPacket) + ".ts";
            fs.open("test/" + name, std::ios::out | std::ios::binary);
            if (!fs.is_open()) {
                fprintf(stderr, "open %s failed\n", name.c_str());
                return -1;
            }

            fs.read(reinterpret_cast<char *>(transportStreamBuffer), TRANSPORT_STREAM_PACKETS_SIZE);
            size = fs.gcount();
            if (size != TRANSPORT_STREAM_PACKETS_SIZE) {
                fprintf(stderr, "没读到一个ts包的大小，read size = %d\n", size);
                return -1;
            }
        }
        ReadStream rs(transportStreamBuffer, size);
        ret = demux.readFrame(rs);
        if (ret < 0) {
            fprintf(stderr, "demux.readVideoFrame失败\n");
            return ret;
        }
        if (ret == AUDIO_PID) {
            size = rs.size - rs.position;
            memcpy(buffer + blockBufferSize, rs.currentPtr, size);
            blockBufferSize += size;
            bufferEnd = buffer + blockBufferSize;
            break;
        }

    }

    return 0;
}




