#include "adtsReader.h"

#include <cstdio>
#include <cstring>
#include <thread>
#include "util.h"
#include "readStream.h"


#define min(a, b) ( (a) < (b) ? (a) : (b) )


//int AdtsReader::init(const char *filename) {
//    fs.open(filename, std::ios::out | std::ios::binary);
//    if (!fs.is_open()) {
//        fprintf(stderr, "open %s failed\n", filename);
//        return -1;
//    }
//
//
//    //最大ADTS头是9，没有差错校验就是7
//    buffer = new uint8_t[MAX_BUFFER_SIZE];
//    fs.read(reinterpret_cast<char *>(buffer), MAX_BUFFER_SIZE);
//    blockBufferSize = fs.gcount();
//
//
//    if (blockBufferSize < MAX_HEADER_SIZE) {
//        fprintf(stderr, "数据不完整\n");
//        return -1;
//    }
//
//    if (memcmp(buffer, "ID3", 3) == 0) {
//        fprintf(stderr, "不支持解析ID3\n");
//        return -1;
//    } else if (memcmp(buffer, "ADIF", 4) == 0) {
//        fprintf(stderr, "不支持解析ADIF格式\n");
//        return -1;
//    }
//    return 0;
//}

int AdtsReader::init1(const std::string &dir, uint32_t transportStreamPacketNumber) {
    path = dir;
    currentPacket = transportStreamPacketNumber;
    std::string name = "/test" + std::to_string(currentPacket) + ".ts";
    printf("读取%s文件 audio init\n", name.c_str());
    fs.open(path + name, std::ios::out | std::ios::binary);
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
    // fs.read(reinterpret_cast<char *>(buffer), MAX_BUFFER_SIZE);
    blockBufferSize = 0;
    bufferEnd = nullptr;
    bufferPosition = nullptr;
    audioDecodeFrameNumber = 0;
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
    /*转换成微秒*/
    header.interval = (int) (1024.0 / (double) header.sample_rate * 1000.0);
    audioDecodeFrameNumber += 1024;
}

int AdtsReader::getTransportStreamData() {
    int ret;
    std::string name;
    //uint8_t offset = 0;
    uint32_t size = 0;
    while (true) {
        fs.read(reinterpret_cast<char *>(transportStreamBuffer + size), TRANSPORT_STREAM_PACKETS_SIZE - size);
        size += fs.gcount();
        if (size == 0) {
            uint32_t pos = fs.tellg();
            /*表示这个文件读完了，读下一个*/
            fs.close();
            name = "/test" + std::to_string(++currentPacket) + ".ts";
            printf("读取%s文件 读取的size = %d audio\n", name.c_str(), size);
            fs.open(path + name, std::ios::out | std::ios::binary);
            if (!fs.is_open()) {
                fprintf(stderr, "读取%s失败 audio\n", name.c_str());
                /*如果是走到这里打开下一个文件失败，表示上个文件还没读完，继续读上个文件*/
                name = "/test" + std::to_string(--currentPacket) + ".ts";
                fs.open(path + name, std::ios::out | std::ios::binary);
                /*返回到上次读取到的位置*/
                fs.seekg(pos);
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
                continue;
            }

            fs.read(reinterpret_cast<char *>(transportStreamBuffer), TRANSPORT_STREAM_PACKETS_SIZE);
            size = fs.gcount();
            if (size != TRANSPORT_STREAM_PACKETS_SIZE) {
                fprintf(stderr, "没读到一个ts包的大小，read size = %d\n", size);
                return -1;
            }
        } else if (size < TRANSPORT_STREAM_PACKETS_SIZE) {
            fs.clear();
            fprintf(stderr, "size < TRANSPORT_STREAM_PACKETS_SIZE %d,  audio currentPacket = %d\n", size,
                    currentPacket);
            // offset = size;
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            continue;
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
        size = 0;
    }

    return 0;
}




