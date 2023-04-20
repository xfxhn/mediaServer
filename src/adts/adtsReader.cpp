#include "adtsReader.h"

#include <cstdio>
#include <cstring>
#include <thread>
#include "utils/util.h"
#include "bitStream/readStream.h"




int AdtsReader::init() {
    buffer = new uint8_t[MAX_BUFFER_SIZE];
    blockBufferSize = 0;
    return 0;
}






void AdtsReader::putData(uint8_t *data, uint32_t size) {
    memcpy(buffer + blockBufferSize, data, size);
    blockBufferSize += size;
    bufferEnd = buffer + blockBufferSize;
}

void AdtsReader::resetBuffer() {
    if (resetFlag) {
        uint32_t remainingByte = bufferEnd - bufferPosition;
        blockBufferSize = remainingByte;
        memcpy(buffer, bufferPosition, remainingByte);
        resetFlag = false;
    }

}

int AdtsReader::getParameter() {
    int ret;

    if (buffer == nullptr) {
        fprintf(stderr, "请初始化\n");
        return -1;
    }
    if (blockBufferSize < MAX_HEADER_SIZE) {
        return 0;
    }


    ReadStream rs(buffer, blockBufferSize);
    if (rs.getMultiBit(12) != 0xFFF) {
        fprintf(stderr, "格式不对,不等于0xFFF\n");
        return -1;
    }

    ret = parameter.adts_fixed_header(rs);
    if (ret < 0) {
        fprintf(stderr, "解析adts fixed header失败\n");
        return -1;
    }
    parameter.adts_variable_header(rs);

    uint16_t frameLength = parameter.frame_length;

    if (blockBufferSize >= frameLength) {
        parameter.data = &buffer[MAX_HEADER_SIZE];
        parameter.size = frameLength - MAX_HEADER_SIZE;
        bufferPosition = buffer + frameLength;

        disposeAudio(parameter, parameter.data, parameter.size);
        resetFlag = true;
        return 1;
    }
    return 0;
}

int AdtsReader::getAudioFrame2(AdtsHeader &header) {
    int ret;

    if (buffer == nullptr) {
        fprintf(stderr, "请初始化\n");
        return -1;
    }
    if (blockBufferSize < MAX_HEADER_SIZE) {
        return 0;
    }

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

    if (blockBufferSize >= frameLength) {
        header.data = &buffer[MAX_HEADER_SIZE];
        header.size = frameLength - MAX_HEADER_SIZE;
        bufferPosition = buffer + frameLength;

        disposeAudio(header, header.data, header.size);
        resetFlag = true;
    }


    return 0;
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
    header.finishFlag = true;
}


AdtsReader::~AdtsReader() {
    if (buffer) {
        delete[] buffer;
        buffer = nullptr;
    }
}







