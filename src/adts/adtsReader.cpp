#include "adtsReader.h"

#include <cstdio>
#include <cstring>

#include "readStream.h"
#include "adtsHeader.h"


#define min(a, b) ( (a) < (b) ? (a) : (b) )
struct AVRational {
    int num;
    int den;
};

static uint64_t av_rescale_q(uint64_t a, const AVRational &bq, const AVRational &cq) {
    //(1 / 25) / (1 / 1000);
    int64_t b = bq.num * cq.den;
    int64_t c = cq.num * bq.den;
    return a * b / c;  //25 * (1000 / 25)  把1000分成25份，然后当前占1000的多少
}

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

void AdtsReader::reset() {
    fs.seekg(0);
    fs.read(reinterpret_cast<char *>(buffer), MAX_BUFFER_SIZE);
    blockBufferSize = fs.gcount();
    fillByteSize = 0;
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

AdtsReader::~AdtsReader() {
    if (buffer) {
        delete[] buffer;
        buffer = nullptr;
    }
    fs.close();
}

int AdtsReader::putAACData(AdtsHeader &header, uint8_t *data, uint32_t size) {
    header.data = data;
    header.size = size;
    header.duration += (1024.0 / header.sample_rate);
    header.dts = av_rescale_q(audioDecodeFrameNumber, {1, static_cast<int>(header.sample_rate)}, {1, 1000});
    header.pts = header.dts;
    audioDecodeFrameNumber += 1024;
    return 0;
}


