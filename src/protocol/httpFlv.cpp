
#include "httpFlv.h"
#include <cstdio>
#include "adtsReader.h"
#include "NALReader.h"

#include "writeStream.h"
#include "FLVHeader.h"
#include "FLVTagHeader.h"
#include "FLVScriptTag.h"
#include "FLVVideoTag.h"
#include "FLVAudioTag.h"


#define TAG_SIZE 4
#define TAG_HEADER_SIZE 11

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
    audioBuffer = new uint8_t[AdtsReader::MAX_BUFFER_SIZE];
    videoBuffer = new uint8_t[NALReader::MAX_BUFFER_SIZE];

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

    memset(videoBuffer, 0, 1024);

    sprintf(reinterpret_cast<char *>(videoBuffer),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: video/x-flv\r\n"
            "Connection: keep-alive\r\n"
            "\r\n"
    );
    ret = TcpSocket::sendData(clientSocket, videoBuffer, (int) strlen(reinterpret_cast<char *>(videoBuffer)));
    if (ret < 0) {
        fprintf(stderr, "发送数据失败\n");
        return ret;
    }


    WriteStream ws(videoBuffer, FLVHeader::size);
    FLVHeader header;
    header.write(ws);
    ret = TcpSocket::sendData(clientSocket, videoBuffer, (int) ws.bufferSize);
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
    WriteStream ws(videoBuffer, TAG_SIZE + TAG_HEADER_SIZE + META_SIZE);
    /*写入上个tag的大小*/
    ws.writeMultiBit(32, previousTagSize);

    FLVTagHeader tagHeader;
    tagHeader.setConfig(SCRIPT, META_SIZE, 0);
    tagHeader.write(ws);


    FLVScriptTag scriptTag;
    scriptTag.setConfig(packet.sps.PicWidthInSamplesL, packet.sps.PicHeightInSamplesL, packet.sps.fps,
                        packet.aacHeader.sample_rate, packet.aacHeader.channel_configuration);
    scriptTag.write(ws);


    ret = TcpSocket::sendData(clientSocket, videoBuffer, (int) ws.bufferSize);
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
    WriteStream ws(videoBuffer, TAG_SIZE + TAG_HEADER_SIZE + AVC_CONFIG_SIZE + packet.spsSize + packet.ppsSize);
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

    ret = TcpSocket::sendData(clientSocket, videoBuffer, (int) ws.bufferSize);
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
    WriteStream ws(videoBuffer, TAG_SIZE + TAG_HEADER_SIZE + AAC_CONFIG_SIZE);
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
    ret = TcpSocket::sendData(clientSocket, videoBuffer, (int) ws.bufferSize);
    if (ret < 0) {
        fprintf(stderr, "发送数据失败\n");
        return ret;
    }
    previousTagSize = TAG_HEADER_SIZE + AAC_CONFIG_SIZE;

    return 0;
}

int HttpFlv::sendData() {
    int ret;
    AVPacket packet;
    ret = packet.init(dir, transportStreamPacketNumber);
    if (ret < 0) {
        fprintf(stderr, "packet.init 初始化失败\n");
        return ret;
    }

    while (true) {
        ret = packet.readFrame(package);
        if (ret < 0) {
            fprintf(stderr, "packet.readFrame 失败\n");
            return -1;
        }
        if (package->type == "video") {
            ret = sendVideoData();
            if (ret < 0) {
                fprintf(stderr, "sendVideoData 失败\n");
                return ret;
            }
        } else if (package->type == "audio") {
            ret = sendAudioData();
            if (ret < 0) {
                fprintf(stderr, "sendAudioData 失败\n");
                return ret;
            }
        }

    }
    return 0;
}

int HttpFlv::sendAudioData() {

    int ret;

    constexpr int AAC_CONFIG_SIZE = 2;
    WriteStream ws(audioBuffer, TAG_SIZE + TAG_HEADER_SIZE + AAC_CONFIG_SIZE + package->size);
    /*写入上个tag的大小*/
    ws.writeMultiBit(32, previousTagSize);

    FLVTagHeader tagHeader;
    tagHeader.setConfig(AUDIO, AAC_CONFIG_SIZE + package->size, package->pts);
    tagHeader.write(ws);


    FLVAudioTag audioTag;
    audioTag.setConfig(1);
    audioTag.writeData(ws, package->data2, package->size);


    ret = TcpSocket::sendData(clientSocket, audioBuffer, (int) ws.bufferSize);
    if (ret < 0) {
        fprintf(stderr, "发送数据失败\n");
        return ret;
    }
    previousTagSize = TAG_HEADER_SIZE + AAC_CONFIG_SIZE + package->size;

    return 0;
}


int HttpFlv::sendVideoData() {
    int ret;
    constexpr int AVC_CONFIG_SIZE = 5;
    WriteStream ws(videoBuffer,
                   TAG_SIZE + TAG_HEADER_SIZE + AVC_CONFIG_SIZE + package->size + package->data1.size() * 4);
    /*写入上个tag的大小*/
    ws.writeMultiBit(32, previousTagSize);

    FLVTagHeader tagHeader;
    tagHeader.setConfig(VIDEO, AVC_CONFIG_SIZE + package->size + package->data1.size() * 4, package->dts);
    tagHeader.write(ws);


    FLVVideoTag videoTag;
    /* 5 */
    /* + (int) package->fps * 3)*/
    videoTag.setConfig(package->idrFlag ? 1 : 2, AVC_NALU, (package->pts + 80 - package->dts));
    videoTag.writeData(ws);

    for (auto &i: package->data1) {
        ws.writeMultiBit(32, i.nalUintSize);
        memcpy(ws.currentPtr, i.data, i.nalUintSize);
        ws.setBytePtr(i.nalUintSize);

    }
    ret = TcpSocket::sendData(clientSocket, videoBuffer, (int) ws.bufferSize);
    if (ret < 0) {
        fprintf(stderr, "发送数据失败\n");
        return ret;
    }

    previousTagSize = TAG_HEADER_SIZE + AVC_CONFIG_SIZE + package->size + package->data1.size() * 4;
    return 0;
}

HttpFlv::~HttpFlv() {
    AVPacket::freePacket(package);
    delete[] audioBuffer;
    delete[] videoBuffer;
}
