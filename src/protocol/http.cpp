
#include "http.h"

#include <fstream>
//#include <cstring>


#include "FLVHeader.h"
#include "FLVTagHeader.h"
#include "FLVScriptTag.h"
#include "FLVVideoTag.h"
#include "FLVAudioTag.h"
#include "writeStream.h"
#include "util.h"


#define TAG_SIZE 4
#define TAG_HEADER_SIZE 11


uint8_t Http::response[1024]{0};

int Http::init(SOCKET socket) {
    clientSocket = socket;
    package = AVPacket::allocPacket();


    audioBuffer = new uint8_t[AdtsReader::MAX_BUFFER_SIZE];
    videoBuffer = new uint8_t[NALReader::MAX_BUFFER_SIZE];
    return 0;
}


int Http::parse(std::string &packet, const std::string &data) {
    int ret;


    std::string method, path, version;

    std::vector<std::string> list = split(data, "\r\n");

    std::istringstream iss(list.front());
    iss >> method >> path >> version;

    list.erase(list.begin());
    request = getObj(list, ":");

    if (method != "GET") {
        fprintf(stderr, "method = %s\n", method.c_str());
        responseData(405, "Method Not Allowed");
        return -1;
    }


    std::filesystem::path source("." + path);

    if (source.extension() == ".m3u8" || source.extension() == ".ts") {
        ret = hls.init(source, clientSocket);
        if (ret < 0) {
            responseData(500, "错误");
            return -1;
        }
        ret = hls.disposeHls();
        if (ret < 0) {
            responseData(500, "错误");
            return -1;
        }
    } else if (source.extension() == ".flv") {
        ret = flv.init(source, clientSocket);
        if (ret < 0) {
            responseData(500, "错误");
            return -1;
        }
        ret = flv.disposeFlv();
        if (ret < 0) {
            responseData(500, "错误");
            return -1;
        }
    } else {
        fprintf(stderr, "不支持这种类型\n");
        responseData(415, "Unsupported Media Type");
        return -1;
    }



//    std::string mimeType;
//    if (source.extension() == ".m3u8") {
//        mimeType = "application/x-mpegurl";
//        ret = disposeTransStream(source.string(), mimeType);
//        if (ret < 0) {
//            fprintf(stderr, "disposeTransStream 失败\n");
//            return ret;
//        }
//    } else if (source.extension() == ".ts") {
//        mimeType = "video/mp2t";
//        hls.disposeHls();
//        ret = disposeTransStream(source.string(), mimeType);
//        if (ret < 0) {
//            fprintf(stderr, "disposeTransStream 失败\n");
//            return ret;
//        }
//    } else if (source.extension() == ".flv") {
//        mimeType = "video/x-flv";
//        ret = initFLV(source, mimeType);
//        if (ret < 0) {
//            fprintf(stderr, "disposeTransStream 失败\n");
//            return ret;
//        }
//    } else {
//        fprintf(stderr, "不支持这种类型\n");
//        responseData(415, "Unsupported Media Type");
//        return -1;
//    }


    return 0;
}


int Http::initFLV(std::filesystem::path &path, const std::string &mimeType) {
    int ret;

    path.replace_extension("");
    if (!std::filesystem::exists(path)) {
        fprintf(stderr, "目录不存在\n");
        responseData(404, "找不到流");
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




    sendHeader(mimeType);
    sendMetadata();
    sendSequenceHeader();
    sendData();

    return 0;
}


int Http::sendHeader(const std::string &mimeType) {
    int ret;

    memset(response, 0, 1024);

    sprintf(reinterpret_cast<char *>(response),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: %s\r\n"
            "Connection: keep-alive\r\n"
            "\r\n",
            mimeType.c_str()
    );
    ret = TcpSocket::sendData(clientSocket, response, (int) strlen(reinterpret_cast<char *>(response)));
    if (ret < 0) {
        fprintf(stderr, "发送数据失败\n");
        return ret;
    }

    /*包含了最后四个字节=0  previousTagSize*/
    WriteStream ws(response, FLVHeader::size);
    FLVHeader header;
    header.write(ws);
    ret = TcpSocket::sendData(clientSocket, response, 9);
    if (ret < 0) {
        fprintf(stderr, "发送数据失败 FLV header\n");
        return ret;
    }
    previousTagSize = 0;
    return 0;
}

int Http::sendMetadata() {
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
    WriteStream ws(response, TAG_SIZE + TAG_HEADER_SIZE + metaSize);
    /*写入上个tag的大小*/
    ws.writeMultiBit(32, previousTagSize);

    FLVTagHeader tagHeader;
    tagHeader.setConfig(SCRIPT, metaSize, 0);
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

    previousTagSize = TAG_HEADER_SIZE + metaSize;
    return 0;
}

int Http::sendSequenceHeader() {
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

int Http::sendVideoSequenceHeader(AVPacket &packet) {
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

int Http::sendAudioSequenceHeader(AVPacket &packet) {
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
    const uint8_t profile = packet.aacHeader.ID == 1 ? packet.aacHeader.profile + 1 : packet.aacHeader.profile;
    audioTag.setConfigurationRecord(profile, packet.aacHeader.sampling_frequency_index,
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

int Http::sendData() {
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

int Http::sendAudioData() {

    int ret;

    constexpr int AAC_CONFIG_SIZE = 2;
    WriteStream ws(audioBuffer, TAG_SIZE + TAG_HEADER_SIZE + AAC_CONFIG_SIZE + package->size);
    /*写入上个tag的大小*/
    ws.writeMultiBit(32, previousTagSize);

    FLVTagHeader tagHeader;
    tagHeader.setConfig(AUDIO, AAC_CONFIG_SIZE, package->pts);
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

int Http::sendVideoData() {
    int ret;
    constexpr int AVC_CONFIG_SIZE = 5;
    WriteStream ws(videoBuffer,
                   TAG_SIZE + TAG_HEADER_SIZE + AVC_CONFIG_SIZE + package->size + package->data1.size() * 4);
    /*写入上个tag的大小*/
    ws.writeMultiBit(32, previousTagSize);

    FLVTagHeader tagHeader;
    tagHeader.setConfig(VIDEO, AVC_CONFIG_SIZE + package->size, package->dts);
    tagHeader.write(ws);


    FLVVideoTag videoTag;
    /* 5 */
    videoTag.setConfig(package->idrFlag ? 1 : 2, AVC_NALU, package->pts - package->dts);
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

int Http::disposeTransStream(const std::string &path, const std::string &mimeType) {
    int ret;
    std::ifstream fs(path, std::ios::binary | std::ios::ate);
    if (!fs.good()) {
        fprintf(stderr, "当前文件不可读 %s\n", ("." + path).c_str());
        responseData(500, "当前文件不可读");
        return -1;
    }

    uint32_t size = fs.tellg();
    fs.seekg(0, std::ios::beg);


    std::ostringstream oss;
    oss << "HTTP/1.1 200 OK\r\n";
    oss << "Content-Length: " << size << "\r\n";
    oss << "Content-Type: " << mimeType << "\r\n";
    oss << "Connection: keep-alive\r\n";
    oss << "\r\n";
    oss << fs.rdbuf();

    std::string res = oss.str();
    ret = TcpSocket::sendData(clientSocket, reinterpret_cast<uint8_t *>(const_cast<char *>(res.c_str())),
                              static_cast<int>(res.size()));
    if (ret < 0) {
        fprintf(stderr, "发送数据失败\n");
        return ret;
    }
    return 0;
}


int Http::responseData(int status, const std::string &msg) const {
    memset(response, 0, 1024);
    std::string body = "<html><body><h1>" + msg + "</h1></body></html>";
    sprintf(reinterpret_cast<char *>(response),
            "HTTP/1.1 %d %s\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: %zu\r\n"
            "\r\n"
            "%s",
            status, msg.c_str(),
            body.size(),
            body.c_str()
    );

    int ret = TcpSocket::sendData(clientSocket, response, (int) strlen(reinterpret_cast<char *>(response)));
    if (ret < 0) {
        fprintf(stderr, "发送数据失败\n");
        return ret;
    }
    return 0;
}


Http::~Http() {
    AVPacket::freePacket(package);
}




