#include "server.h"


#include <cstdio>
//#include "util.h"
//#include "audioSpecificConfig.h"
//#include "writeStream.h"
//#include "readStream.h"
#include "rtspTask.h"

//#define RTP_PAYLOAD_TYPE_H264   96
//#define RTP_PAYLOAD_TYPE_AAC    97


//class MyTask : public Task {
//public:
//
//    TcpSocket &rtsp;
//private:
//    const char *videoFilename{""};
//    const char *audioFilename{""};
//    AnalysisData ad;
//
//
//    RtpPacket videoPacket;
//    RtpPacket audioPacket;
//public:
//    explicit MyTask(TcpSocket &rtsp_) : rtsp(rtsp_) {
//
//    }
//
//    int initVideo(const char *filename) {
//        int ret;
//        videoFilename = filename;
//        NALReader reader;
//        ret = reader.init(filename);
//        if (ret < 0) {
//            fprintf(stderr, "init video failed\n");
//            return ret;
//        }
//
//        uint32_t spsSize = 0;
//        uint32_t ppsSize = 0;
//        ret = reader.getSpsAndPps(spsSize, ppsSize);
//        if (ret < 0) {
//            fprintf(stderr, "获取sps和pps失败\n");
//            return ret;
//        }
//
//        /*这里传输了SPS和PPS，其实可以在传输数据的时候不带SPS和PPS*/
//        ad.sprop_parameter_sets = GenerateSpropParameterSets(reader.spsData, spsSize, reader.ppsData, ppsSize);
//
//        ad.profileLevelId += decimalToHex(reader.sps.profile_idc);
//        ad.profileLevelId += decimalToHex(reader.sps.compatibility);
//        ad.profileLevelId += decimalToHex(reader.sps.level_idc);
//
//
//        videoPacket.init(NALReader::MAX_BUFFER_SIZE, RTP_PAYLOAD_TYPE_H264);
//        return ret;
//    }
//
//    int initAudio(const char *filename) {
//        int ret;
//        audioFilename = filename;
//        AdtsReader reader;
//        ret = reader.init(filename);
//        if (ret < 0) {
//            fprintf(stderr, "init audio failed\n");
//            return ret;
//        }
//
//        bool flag = true;
//        ret = reader.getAudioFrame(ad.header, flag);
//        if (ret < 0) {
//            fprintf(stderr, "获取audio frame失败\n");
//            return ret;
//        }
//
//        //aacConfig 大小
//        constexpr int size = 2;
//        uint8_t buffer[size];
//        WriteStream ws(buffer, size);
//
//        AudioSpecificConfig AACConfig;
//        ret = AACConfig.setConfig(ad.header.ID == 1 ? ad.header.profile + 1 : ad.header.profile,
//                                  ad.header.sampling_frequency_index,
//                                  ad.header.channel_configuration);
//        if (ret < 0) {
//            return ret;
//        }
//
//        AACConfig.write(ws);
//        for (uint8_t i: buffer) {
//            ad.aacConfig += decimalToHex(i);
//        }
//        audioPacket.init(AdtsReader::MAX_BUFFER_SIZE, RTP_PAYLOAD_TYPE_AAC);
//        return 0;
//    }
//
///*多个不同请求   比如有rtsp 和 http*/
//    int run() override {
//        int ret;
//
//
//        char buffer[TcpSocket::MAX_BUFFER]{0};
//        int length = 0;
//        std::string packet;
//        while (true) {
//            /*length本次获取的数据大小*/
//            ret = rtsp.receive(buffer, length);
//            if (ret < 0) {
//                fprintf(stderr, "rtsp.receive failed\n");
//                return ret;
//            }
//
//            // 将缓冲区中的数据添加到字符串对象中
//            packet.append(buffer, length);
//            // 查找分隔符在字符串中的位置
//            std::string::size_type pos = packet.find("\r\n\r\n");
//            while (pos != std::string::npos) {
//                // 截取出一个完整的数据包（不含分隔符）
//                std::string data = packet.substr(0, pos);
//                // 去掉已处理的部分（含分隔符）
//                packet.erase(0, pos + 4);
//
//                /*处理请求，接收客户端发过来的数据*/
//
//                // 继续查找下一个分隔符位置
//                pos = packet.find("\r\n\r\n");
//            }
//        }
//
//
//        char resBuffer[1024]{0};
//
//        std::thread *videoThread = nullptr;
//        std::thread *audioThread = nullptr;
//        bool videoThreadFlag = true;
//        bool audioThreadFlag = true;
//        while (videoThreadFlag && audioThreadFlag) {
//            /*char *data = nullptr;
//            int dataSize = 0;*/
//            ret = ad.test(rtsp);
//            if (ret <= 0) {
//                printf("receiveData ret = %d\n", ret);
//                rtsp.closeSocket();
//                break;
//            }
//
//            memset(resBuffer, 0, 1024);
//            int status = ad.getInfo(data, resBuffer);
//            if (status < 0) {
//                delete[] data;
//                fprintf(stderr, "解析数据失败\n");
//                break;
//            }
//
//            ret = rtsp.sendData(reinterpret_cast<uint8_t *>(resBuffer), static_cast<int>(strlen(resBuffer)));
//            if (ret < 0) {
//                delete[] data;
//                fprintf(stderr, "发送数据失败\n");
//                break;
//            }
//
//            //  printf("%s\n", resBuffer);
//
//            delete[] data;
//            if (status == 1) {
//                /*释放资源等等*/
//                fprintf(stderr, "TEARDOWN请求，正常关闭\n");
//                break;
//            }
//            if (status == 2) {
//                /*然后准备播放，由于这里要传输音频和视频数据，这里可以开两个线程推送音视频据，相当于线程里面开启子线程*/
//
//                videoThread = new std::thread(&MyTask::sendVideo, this, std::ref(videoThreadFlag));
//                printf("启动了视频发送线程\n");
//
//                audioThread = new std::thread(&MyTask::sendAudio, this, std::ref(audioThreadFlag));
//                printf("启动了音频发送线程\n");
//
//            }
//        }
//
//        if (videoThread && videoThread->joinable()) {
//            printf("视频线程结束\n");
//            videoThread->join();
//            delete videoThread;
//        }
//        if (audioThread && audioThread->joinable()) {
//            printf("音频线程结束\n");
//            audioThread->join();
//            delete audioThread;
//        }
//
//        printf("finish 这个链接\n");
//
//        return 0;
//    }
//
//
//    int sendVideo(bool &videoThreadFlag) {
//        int ret;
//        bool flag = true;
//        // int num = 0;
//
//        NALReader reader;
//        ret = reader.init(videoFilename);
//        if (ret < 0) {
//            fprintf(stderr, "init video failed\n");
//            videoThreadFlag = false;
//            return ret;
//        }
//        NALPicture *picture = reader.allocPicture();
//        while (flag) {
//            ret = reader.getVideoFrame(picture, flag);
//            if (ret < 0) {
//                fprintf(stderr, "获取视频帧失败\n");
//                videoThreadFlag = false;
//                return ret;
//            }
//
//            for (int i = 0; i < picture->data.size(); ++i) {
//                const Frame &frame = picture->data[i];
//
//                ret = videoPacket.sendVideoFrame(rtsp, frame.data, frame.nalUintSize,
//                                                 i == (picture->data.size() - 1), ad.videoChannel);
//                if (ret < 0) {
//                    fprintf(stderr, "发送视频数据失败\n");
//                    videoThreadFlag = false;
//                    return ret;
//                }
//
//            }
//            // printf("pts = %lld\n", picture->pts);
//            videoPacket.timestamp = picture->pts;
//
//            //++num;
//
//            std::this_thread::sleep_for(std::chrono::milliseconds(40 - 20));
//        }
//        printf("视频发送完成\n");
//        return 0;
//    }
//
//    int sendAudio(bool &audioThreadFlag) {
//        int ret;
//
//        AdtsReader reader;
//        ret = reader.init(audioFilename);
//        if (ret < 0) {
//            fprintf(stderr, "init audio failed\n");
//            return ret;
//        }
//
//
//        AdtsHeader header;
//        bool flag = true;
//        while (flag) {
//            ret = reader.getAudioFrame(header, flag);
//            if (ret < 0) {
//                fprintf(stderr, "获取audio frame失败\n");
//                audioThreadFlag = false;
//                return ret;
//            }
//            ret = audioPacket.sendAudioPacket(rtsp, header.data, header.size, ad.audioChannel);
//            if (ret < 0) {
//                fprintf(stderr, "发送音频包失败\n");
//                audioThreadFlag = false;
//                return ret;
//            }
//
//            audioPacket.timestamp += 1024;
//            if (audioPacket.timestamp >= 4294967295) {
//                audioPacket.timestamp = 0;
//            }
//            //  audioPacket.timestamp = header.pts;
//            std::this_thread::sleep_for(std::chrono::milliseconds(10));
//        }
//        printf("音频发送完成\n");
//        return 0;
//    }
//};

constexpr unsigned short SERVER_RTSP_PORT = 8554;


int Server::init() {

#if _WIN32
    /*windows 需要初始化socket库*/
    static WSADATA wsadata;
    WSAStartup(MAKEWORD(2, 2), &wsadata);
#endif

    rtsp.createSocket();
    rtsp.bindSocket("0.0.0.0", SERVER_RTSP_PORT);
    rtsp.listenPort();

    pool.init(10);
    pool.start();

    return 0;
}

//int Rtsp::initVideo(const char *filename) {
//
//    int ret;
//    videoFilename = filename;
//    NALReader reader;
//
//    ret = reader.init(filename);
//    if (ret < 0) {
//        fprintf(stderr, "init video failed\n");
//        return ret;
//    }
//
//    uint32_t spsSize = 0;
//    uint32_t ppsSize = 0;
//    ret = reader.getSpsAndPps(spsSize, ppsSize);
//    if (ret < 0) {
//        fprintf(stderr, "获取sps和pps失败\n");
//        return ret;
//    }
//
//    /*这里传输了SPS和PPS，其实可以在传输数据的时候不带SPS和PPS*/
//    ad.sprop_parameter_sets = GenerateSpropParameterSets(reader.spsData, spsSize, reader.ppsData, ppsSize);
//
//    ad.profileLevelId += decimalToHex(reader.sps.profile_idc);
//    ad.profileLevelId += decimalToHex(reader.sps.compatibility);
//    ad.profileLevelId += decimalToHex(reader.sps.level_idc);
//
//
//    videoPacket.init(NALReader::MAX_BUFFER_SIZE, RTP_PAYLOAD_TYPE_H264);
//    return ret;
//}
//
//
//int Rtsp::initAudio(const char *filename) {
//    int ret;
//    audioFilename = filename;
//    AdtsReader reader;
//    ret = reader.init(filename);
//    if (ret < 0) {
//        fprintf(stderr, "init audio failed\n");
//        return ret;
//    }
//
//    bool flag = true;
//    ret = reader.getAudioFrame(ad.header, flag);
//    if (ret < 0) {
//        fprintf(stderr, "获取audio frame失败\n");
//        return ret;
//    }
//
//    //aacConfig 大小
//    constexpr int size = 2;
//    uint8_t buffer[size];
//    WriteStream ws(buffer, size);
//
//    AudioSpecificConfig AACConfig;
//    ret = AACConfig.setConfig(ad.header.ID == 1 ? ad.header.profile + 1 : ad.header.profile,
//                              ad.header.sampling_frequency_index,
//                              ad.header.channel_configuration);
//    if (ret < 0) {
//        return ret;
//    }
//
//    AACConfig.write(ws);
//    for (uint8_t i: buffer) {
//        ad.aacConfig += decimalToHex(i);
//    }
//    audioPacket.init(AdtsReader::MAX_BUFFER_SIZE, RTP_PAYLOAD_TYPE_AAC);
//    return 0;
//}

int Server::start() {
    int ret;


    while (true) {
        SOCKET clientSocket = rtsp.acceptClient();
        if (clientSocket < 0) {
            fprintf(stderr, "accept 失败\n");
            return -1;
        }
        printf("client ip:%s,client port:%d\n", rtsp.clientIp, rtsp.clientPort);


        RtspTask *task = new RtspTask(clientSocket); // NOLINT(modernize-use-auto)
        pool.addTask(task);




        /*如果是拉流和推流都是用的同一个端口的话，至少这里就要开辟线程，异步处理*/
        /*然后这个线程，对当前这个链接进来的用户做服务，直到服务器主动关闭，或者用户主动关闭这个链接，然后回收线程*/

//        std::thread *videoThread = nullptr;
//        std::thread *audioThread = nullptr;
//        bool videoThreadFlag = true;
//        bool audioThreadFlag = true;
//        while (videoThreadFlag && audioThreadFlag) {
//            char *data = nullptr;
//            int dataSize = 0;
//            ret = TcpSocket::receiveData(clientSocket, data, dataSize);
//            if (ret <= 0) {
//                printf("receiveData ret = %d\n", ret);
//                TcpSocket::closeSocket(clientSocket);
//                break;
//            }
//
//            memset(resBuffer, 0, 1024);
//            int status = ad.getInfo(data, resBuffer);
//            if (status < 0) {
//                delete[] data;
//                fprintf(stderr, "解析数据失败\n");
//                break;
//            }
//            ret = TcpSocket::sendData(clientSocket, resBuffer, static_cast<int>(strlen(resBuffer)));
//            if (ret < 0) {
//                delete[] data;
//                fprintf(stderr, "发送数据失败\n");
//                break;
//            }
//
//            //  printf("%s\n", resBuffer);
//
//            delete[] data;
//            if (status == 1) {
//                /*释放资源等等*/
//                fprintf(stderr, "TEARDOWN请求，正常关闭\n");
//                break;
//            }
//            if (status == 2) {
//                /*然后准备播放，由于这里要传输音频和视频数据，这里可以开两个线程推送音视频据，相当于线程里面开启子线程*/
//
//                videoThread = new std::thread(&Rtsp::sendVideo, this, clientSocket, std::ref(videoThreadFlag),
//                                              ad.videoChannel);
//                printf("启动了视频发送线程\n");
//
//                audioThread = new std::thread(&Rtsp::sendAudio, this, clientSocket, std::ref(audioThreadFlag),
//                                              ad.audioChannel);
//                printf("启动了音频发送线程\n");
//
//            }
//
//
//        }
//        if (videoThread != nullptr && videoThread->joinable()) {
//            printf("视频线程结束\n");
//            videoThread->join();
//            delete videoThread;
//        }
//        if (audioThread) {
//            printf("音频线程结束\n");
//            audioThread->join();
//            delete audioThread;
//        }
//
//        printf("finish 这个链接\n");
    }


    return 0;
}











