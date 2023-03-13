
#ifndef RTSP_RTSPTASK_H
#define RTSP_RTSPTASK_H


#include "task.h"
#include "rtsp.h"
#include "TcpSocket.h"


class RtspTask : public Task {

private:
    /* TcpSocket &rtsp;*/
    SOCKET clientSocket;
    const char *videoFilename{""};
    const char *audioFilename{""};


public:
    explicit RtspTask(SOCKET socket) : clientSocket(socket) {

    }



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

    /*多个不同请求   比如有rtsp 和 http*/
    int run() override {
        int ret;

        Rtsp rtsp;
        ret = rtsp.init(clientSocket);
        if (ret < 0) {
            return ret;
        }

        char buffer[TcpSocket::MAX_BUFFER]{0};
        int length = 0;
        std::string packet;

        while (rtsp.stopFlag) {
            /*length本次获取的数据大小*/
            ret = TcpSocket::receive(clientSocket, buffer, length);
            if (ret < 0) {
                fprintf(stderr, "rtsp.receive failed\n");
                return ret;
            }

            /*视频发送错误了，退出这个线程*/
            if (rtsp.videoSendError) {
                fprintf(stderr, "视频发送错误\n");
                return -1;
            }
            if (rtsp.audioSendError) {
                fprintf(stderr, "音频发送错误\n");
                return -1;
            }
            // 将缓冲区中的数据添加到字符串对象中
            packet.append(buffer, length);
            // 查找分隔符在字符串中的位置
            std::string::size_type pos = packet.find("\r\n\r\n");
            while (pos != std::string::npos) {
                // 截取出一个完整的数据包（不含分隔符）
                std::string data = packet.substr(0, pos);
                // 去掉已处理的部分（含分隔符）
                packet.erase(0, pos + 4);

                /*处理请求，接收客户端发过来的数据*/
                ret = rtsp.parseRtsp(packet, data);
                if (ret < 0) {
                    fprintf(stderr, "解析rtsp失败\n");
                    return ret;
                }


                // 继续查找下一个分隔符位置
                pos = packet.find("\r\n\r\n");
            }
        }

        return 0;


    }

    ~RtspTask() override {
        /*退出的时候关闭这个连接*/
        TcpSocket::closeSocket(clientSocket);
    }

};


#endif //RTSP_RTSPTASK_H
