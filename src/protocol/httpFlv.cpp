
#include "httpFlv.h"

#include "adtsReader.h"
#include "NALReader.h"

#include "writeStream.h"
#include "FLVHeader.h"
#include "FLVTagHeader.h"
#include "FLVScriptTag.h"
#include "FLVVideoTag.h"
#include "FLVAudioTag.h"

#include "AVPacket.h"

#define TAG_SIZE 4
#define TAG_HEADER_SIZE 11

int HttpFlv::init(std::filesystem::path &path, SOCKET socket) {

    path.replace_extension("");
    if (!std::filesystem::exists(path)) {
        fprintf(stderr, "目录不存在\n");
        return -1;
    }


    for (const std::filesystem::directory_entry &entry: std::filesystem::directory_iterator(path)) {
        std::string extension = entry.path().extension().string();
        if (extension == ".ts") {
            std::string filename = entry.path().filename().string();
            std::size_t start = filename.find("test") + 4;
            std::size_t end = filename.find(".ts");
            int number = std::stoi(filename.substr(start, end - start));
            if (number > transportStreamPacketNumber) {
                transportStreamPacketNumber = number;
                dir = path.string();
            }
        }
    }
    clientSocket = socket;
    audioBuffer = new uint8_t[AdtsReader::MAX_BUFFER_SIZE];
    videoBuffer = new uint8_t[NALReader::MAX_BUFFER_SIZE];
    return 0;
}

int HttpFlv::disposeFlv() {
    int ret;
    sendHeader();
    sendMetadata();
    sendSequenceHeader();
    sendData();
    return 0;
}

int HttpFlv::sendHeader() {
    int ret;

    memset(videoBuffer, 0, 1024);

    sprintf(reinterpret_cast<char *>(videoBuffer),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: %s\r\n"
            "Connection: keep-alive\r\n"
            "\r\n",
            mimeType.c_str()
    );
    ret = TcpSocket::sendData(clientSocket, videoBuffer, (int) strlen(reinterpret_cast<char *>(videoBuffer)));
    if (ret < 0) {
        fprintf(stderr, "发送数据失败\n");
        return ret;
    }

    /*包含了最后四个字节=0  previousTagSize*/
    WriteStream ws(videoBuffer, FLVHeader::size);
    FLVHeader header;
    header.write(ws);
    ret = TcpSocket::sendData(clientSocket, videoBuffer, 9);
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

    constexpr int metaSize = 138;
    WriteStream ws(videoBuffer, TAG_SIZE + TAG_HEADER_SIZE + metaSize);
    /*写入上个tag的大小*/
    ws.writeMultiBit(32, previousTagSize);

    FLVTagHeader tagHeader;
    tagHeader.setConfig(SCRIPT, metaSize, 0);
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

    previousTagSize = TAG_HEADER_SIZE + metaSize;
    return 0;
}
