
#include "httpFlv.h"
#include <cstdio>

#include "bitStream/writeStream.h"
#include "flashVideo/FLVHeader.h"
#include "flashVideo/FLVTagHeader.h"
#include "flashVideo/FLVScriptTag.h"
#include "flashVideo/FLVVideoTag.h"
#include "flashVideo/FLVAudioTag.h"

#include "log/logger.h"


#define TAG_SIZE 4
#define TAG_HEADER_SIZE 11

static inline uint32_t bswap_32(uint32_t x) {
    x = ((x << 8) & 0xFF00FF00) | ((x >> 8) & 0x00FF00FF);
    return (x >> 16) | (x << 16);
}


int HttpFlv::init(std::filesystem::path &path, SOCKET socket) {

    path.replace_extension("");
    if (!std::filesystem::exists(path)) {
        fprintf(stderr, "目录不存在\n");
        return -1;
    }

    dir = path.string();
    for (const std::filesystem::directory_entry &entry: std::filesystem::directory_iterator(path)) {
        std::string extension = entry.path().extension().string();
        if (extension == ".ts") {
            std::string filename = entry.path().filename().string();
            std::size_t start = filename.find("test") + 4;
            std::size_t end = filename.find(".ts");
            int number = std::stoi(filename.substr(start, end - start));
            if (number > transportStreamPacketNumber) {
                transportStreamPacketNumber = number;
            }
        }
    }
    clientSocket = socket;

    package = AVPacket::allocPacket();


    return 0;
}

int HttpFlv::disposeFlv() {
    int ret;
    ret = sendHeader();
    if (ret < 0) {
        fprintf(stderr, "sendHeader 失败\n");
        return ret;
    }

    ret = sendMetadata();
    if (ret < 0) {
        fprintf(stderr, "sendMetadata 失败\n");
        return ret;
    }
    ret = sendSequenceHeader();
    if (ret < 0) {
        fprintf(stderr, "sendSequenceHeader 失败\n");
        return ret;
    }
    ret = sendData();
    if (ret < 0) {
        fprintf(stderr, "sendData 失败\n");
        return ret;
    }
    return 0;
}

int HttpFlv::sendHeader() {
    int ret;

    memset(response, 0, 1024);

    sprintf(reinterpret_cast<char *>(response),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: video/x-flv\r\n"
            "Connection: keep-alive\r\n"
            "\r\n"
    );
    ret = TcpSocket::sendData(clientSocket, response, (int) strlen(reinterpret_cast<char *>(response)));
    if (ret < 0) {
        fprintf(stderr, "发送数据失败\n");
        return ret;
    }


    WriteStream ws(response, FLVHeader::size);
    FLVHeader header;
    header.write(ws);
    ret = TcpSocket::sendData(clientSocket, response, (int) ws.bufferSize);
    if (ret < 0) {
        fprintf(stderr, "发送数据失败 FLV header\n");
        return ret;
    }
    previousTagSize = 0;


    return 0;
}

int HttpFlv::sendMetadata() {
    int ret;
    AVPacket packet;
    ret = packet.init(dir, transportStreamPacketNumber);
    if (ret < 0) {
        fprintf(stderr, "packet.init 初始化失败\n");
        return ret;
    }
    ret = packet.getParameter();
    if (ret < 0) {
        fprintf(stderr, "packet.getParameter 失败\n");
        return ret;
    }


    constexpr int META_SIZE = 156;
    WriteStream ws(response, TAG_SIZE + TAG_HEADER_SIZE + META_SIZE);
    /*写入上个tag的大小*/
    ws.writeMultiBit(32, previousTagSize);

    FLVTagHeader tagHeader;
    tagHeader.setConfig(SCRIPT, META_SIZE, 0);
    tagHeader.write(ws);


    FLVScriptTag scriptTag;
    scriptTag.setConfig(packet.sps.PicWidthInSamplesL, packet.sps.PicHeightInSamplesL, packet.sps.fps,
                        packet.aacHeader.sample_rate, packet.aacHeader.channel_configuration);
    scriptTag.write(ws);


    ret = TcpSocket::sendData(clientSocket, response, (int) ws.bufferSize);
    if (ret < 0) {
        fprintf(stderr, "发送数据失败 FLV header\n");
        return ret;
    }


    previousTagSize = TAG_HEADER_SIZE + META_SIZE;
    return 0;
}


int HttpFlv::sendSequenceHeader() {
    int ret;
    AVPacket packet;
    ret = packet.init(dir, transportStreamPacketNumber);
    if (ret < 0) {
        fprintf(stderr, "packet.init 初始化失败\n");
        return ret;
    }
    ret = packet.getParameter();
    if (ret < 0) {
        fprintf(stderr, "packet.getParameter 失败\n");
        return ret;
    }
    ret = sendVideoSequenceHeader(packet);
    if (ret < 0) {
        fprintf(stderr, "sendVideoSequenceHeader 失败\n");
        return ret;
    }

    ret = sendAudioSequenceHeader(packet);
    if (ret < 0) {
        fprintf(stderr, "sendAudioSequenceHeader 失败\n");
        return ret;
    }
    return 0;
}

int HttpFlv::sendVideoSequenceHeader(AVPacket &packet) {
    int ret;
    constexpr int AVC_CONFIG_SIZE = 16;
    WriteStream ws(response, TAG_SIZE + TAG_HEADER_SIZE + AVC_CONFIG_SIZE + packet.spsSize + packet.ppsSize);
    /*写入上个tag的大小*/
    ws.writeMultiBit(32, previousTagSize);

    FLVTagHeader tagHeader;
    tagHeader.setConfig(VIDEO, AVC_CONFIG_SIZE + packet.spsSize + packet.ppsSize, 0);
    tagHeader.write(ws);


    FLVVideoTag videoTag;
    /* 5 */
    videoTag.setConfig(1, AVC_sequence_header, 0);
    /* 11 + packet.spsSize + packet.ppsSize */
    videoTag.setConfigurationRecord(packet.sps.profile_idc, packet.sps.level_idc, packet.sps.compatibility);
    videoTag.writeConfig(ws, packet.spsData, packet.spsSize, packet.ppsData, packet.ppsSize);

    ret = TcpSocket::sendData(clientSocket, response, (int) ws.bufferSize);
    if (ret < 0) {
        fprintf(stderr, "发送数据失败\n");
        return ret;
    }
    previousTagSize = TAG_HEADER_SIZE + AVC_CONFIG_SIZE + packet.spsSize + packet.ppsSize;
    return 0;
}

int HttpFlv::sendAudioSequenceHeader(AVPacket &packet) {
    int ret;
    /* write audio Sequence Header */
    constexpr int AAC_CONFIG_SIZE = 4;
    WriteStream ws(response, TAG_SIZE + TAG_HEADER_SIZE + AAC_CONFIG_SIZE);
    /*写入上个tag的大小*/
    ws.writeMultiBit(32, previousTagSize);

    FLVTagHeader tagHeader;
    tagHeader.setConfig(AUDIO, AAC_CONFIG_SIZE, 0);
    tagHeader.write(ws);


    FLVAudioTag audioTag;
    audioTag.setConfig(0);

    audioTag.setConfigurationRecord(packet.aacHeader.profile + 1, packet.aacHeader.sampling_frequency_index,
                                    packet.aacHeader.channel_configuration);
    audioTag.writeConfig(ws);
    ret = TcpSocket::sendData(clientSocket, response, (int) ws.bufferSize);
    if (ret < 0) {
        fprintf(stderr, "发送数据失败\n");
        return ret;
    }
    previousTagSize = TAG_HEADER_SIZE + AAC_CONFIG_SIZE;

    return 0;
}

void HttpFlv::setStopFlag(bool flag) {
    stopFlag = false;
}

int HttpFlv::sendData() {
    int ret;
    AVPacket packet;
    ret = packet.init(dir, transportStreamPacketNumber);
    if (ret < 0) {
        log_error("packet.init 初始化失败");
        return ret;
    }
    /*由外部http控制*/
    while (stopFlag) {
        ret = packet.readFrame(package);
        if (ret < 0) {
            if (ret == AVERROR_EOF) {
                log_info("读取完毕");
                break;
            }
            log_error("packet.readFrame 失败");
            return -1;
        }
        if (package->type == "video") {
            ret = sendVideoData();
            if (ret < 0) {
                log_error("sendVideoData 失败");
                return ret;
            }
        } else if (package->type == "audio") {
            ret = sendAudioData();
            if (ret < 0) {
                log_error("sendAudioData 失败");
                return ret;
            }
        }

    }
    return 0;
}

int HttpFlv::sendAudioData() {

    int ret;
    constexpr int AAC_CONFIG_SIZE = 2;
    WriteStream ws(response, TAG_SIZE + TAG_HEADER_SIZE + AAC_CONFIG_SIZE);
    /*写入上个tag的大小*/
    ws.writeMultiBit(32, previousTagSize);

    FLVTagHeader tagHeader;
    tagHeader.setConfig(AUDIO, AAC_CONFIG_SIZE + package->size, package->pts);
    tagHeader.write(ws);

    FLVAudioTag audioTag;
    audioTag.setConfig(1);
    audioTag.writeData(ws);

    /*先发送flv的壳子*/
    ret = TcpSocket::sendData(clientSocket, response, (int) ws.bufferSize);
    if (ret < 0) {
        log_error("发送数据失败");
        return ret;
    }
    /*在发送具体数据*/
    ret = TcpSocket::sendData(clientSocket, package->data2, (int) package->size);
    if (ret < 0) {
        log_error("发送数据失败");
        return ret;
    }

    previousTagSize = TAG_HEADER_SIZE + AAC_CONFIG_SIZE + package->size;

    return 0;
}


int HttpFlv::sendVideoData() {
    int ret;

    constexpr int AVC_CONFIG_SIZE = 5;
    WriteStream ws(response, TAG_SIZE + TAG_HEADER_SIZE + AVC_CONFIG_SIZE);
    /*写入上个tag的大小*/
    ws.writeMultiBit(32, previousTagSize);

    FLVTagHeader tagHeader;
    tagHeader.setConfig(VIDEO, AVC_CONFIG_SIZE + package->size + package->data1.size() * 4, package->dts);
    tagHeader.write(ws);

    FLVVideoTag videoTag;
    videoTag.setConfig(package->idrFlag ? 1 : 2, AVC_NALU, (package->pts + (int) (package->fps * 2) - package->dts));
    videoTag.writeData(ws);

    /*先发送flv的壳子*/
    ret = TcpSocket::sendData(clientSocket, response, (int) ws.bufferSize);
    if (ret < 0) {
        log_error("发送数据失败");
        return ret;
    }
    for (auto &frame: package->data1) {
        const uint32_t length = bswap_32(frame.nalUintSize);
        ret = TcpSocket::sendData(clientSocket, (uint8_t *) &length, 4);
        if (ret < 0) {
            log_error("发送数据失败");
            return ret;
        }
        ret = TcpSocket::sendData(clientSocket, frame.data, (int) frame.nalUintSize);
        if (ret < 0) {
            log_error("发送数据失败");
            return ret;
        }
    }

    previousTagSize = TAG_HEADER_SIZE + AVC_CONFIG_SIZE + package->size + package->data1.size() * 4;
    return 0;
}

HttpFlv::~HttpFlv() {
    AVPacket::freePacket(package);
}
