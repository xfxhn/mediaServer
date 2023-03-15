
#include "rtsp.h"


#include "util.h"


#include "readStream.h"
#include "writeStream.h"


#define RTP_PAYLOAD_TYPE_H264   96
#define RTP_PAYLOAD_TYPE_AAC    97
enum {
    /*多个nalu在一个rtp包*/
    STAP_A = 24,
    /*单一时间聚合包*/
    STAP_B = 25,
    /*多个时间聚合包 就是可以传输多个nalu，但是编码时间可能不一样*/
    MTAP_16 = 26,
    /*多个时间聚合包 */
    MTAP_24 = 27,
    /*一个nalu容纳不下，分包*/
    FU_A = 28,
    /*第二种分包模式*/
    FU_B = 29
};

int Rtsp::init(SOCKET socket) {
    clientSocket = socket;
    nalUintData = new uint8_t[NALReader::MAX_BUFFER_SIZE];

    // nalReader.init1();

//    aacFrameData = new uint8_t[AdtsReader::MAX_BUFFER_SIZE];
//    aacHeader = aacFrameData;
//    aacData = &aacFrameData[7];

    frame = nalReader.allocPicture();
    if (frame == nullptr) {
        fprintf(stderr, "获取frame失败\n");
        return -1;
    }

    return 0;
}

int Rtsp::getRtpHeader(ReadStream &rs) {
    version = rs.readMultiBit(2);
    padding = rs.readMultiBit(1);
    extension = rs.readMultiBit(1);
    if (extension) {
        fprintf(stderr, "不支持扩展包头\n");
        return -1;
    }
    csrcLen = rs.readMultiBit(4);
    marker = rs.readMultiBit(1);
    payloadType = rs.readMultiBit(7);
    seq = rs.readMultiBit(16);
    timestamp = rs.readMultiBit(32);
    ssrc = rs.readMultiBit(32);
    return 0;
}

int Rtsp::parseRtsp(std::string &packet, const std::string &data) {
    int ret;
    std::vector<std::string> list = split(data, "\r\n");
    char method[20]{0};
    char url[50]{0};
    char version[20]{0};
    char responseBuffer[1024]{0};
    int num = sscanf(list.front().c_str(), "%s %s %s\r\n", method, url, version);
    if (num != 3) {
        fprintf(stderr, "解析method, url, version失败\n");
        return -1;
    }

    list.erase(list.begin());

    std::map<std::string, std::string> obj = getRtspObj(list, ":");

    if (strcmp(method, "OPTIONS") == 0) {
        /*回复客户端当前可用的方法*/
        sprintf(responseBuffer,
                "RTSP/1.0 200 OK\r\n"
                "CSeq: %s\r\n"
                "Date: %s\r\n"
                "Public: OPTIONS, DESCRIBE, SETUP, TEARDOWN, PLAY, PAUSE, ANNOUNCE, RECORD, SET_PARAMETER, GET_PARAMETER\r\n"
                "Server: XiaoFeng\r\n"
                "\r\n",
                obj["CSeq"].c_str(),
                generatorDate().c_str()
        );

        ret = TcpSocket::sendData(clientSocket, reinterpret_cast<uint8_t *>(responseBuffer),
                                  static_cast<int>(strlen(responseBuffer)));
        if (ret < 0) {
            fprintf(stderr, "发送数据失败 -> option\n");
            return ret;
        }
    } else if (strcmp(method, "DESCRIBE") == 0) {
        char sdp[1024]{0};
        /*v=协议版本，一般都是0*/
        /*
         * o=<username> <session-id> <session-version> <nettype> <addrtype> <unicast-address>
         * username：发起者的用户名，不允许存在空格，如果应用不支持用户名，则为-
         * sess-id：会话id，由应用自行定义，规范的建议是NTP(Network Time Protocol)时间戳。
         * sess-version：会话版本，用途由应用自行定义，只要会话数据发生变化时比如编码，sess-version随着递增就行。同样的，规范的建议是NTP时间戳
         * nettype：网络类型，比如IN表示Internet
         * addrtype：地址类型，比如IP4、IV6
         * unicast-address：域名，或者IP地址
         * */
        /*
         * 声明会话的开始、结束时间
         * t=<start-time> <stop-time>
         * 如果<stop-time>是0，表示会话没有结束的边界，但是需要在<start-time>之后会话才是活跃(active)的。
         * 如果<start-time>是0，表示会话是永久的。
         * */

        /*
         * 如果会话级描述为a=control:*，
         * 那么媒体级control后面跟的就是相对于从Content-Base，Content-Location，request URL中去获得基路径
         * request URL指DESCRIBE时的url
         * */

        /*
         * 如果FMTP中的SizeLength参数设置为13，那么它指定了每个NAL单元头部中长度字段的长度为13个比特。
         这意味着，每个NAL单元头部中包含的长度信息将用13位二进制数表示，可以表示的最大值为8191（2的13次方减1）。
        这个具体的设置可能是针对某种特定的视频编码标准或压缩方案而设定的，
         因为不同的编码标准和方案可能需要不同的位数来表示NAL单元的长度信息。
         因此，通过在FMTP中指定SizeLength参数，可以确保解码器正确地解析每个NAL单元，
         并恢复出正确的视频帧数据，以便实现正确的视频播放*/

        /*
profile-level-id=1：表示音频的编码格式是MPEG-4 AAC。
mode=AAC-hbr：表示音频的传输模式是AAC高比特率（high bitrate）。
sizelength=13：表示音频的访问单元（AU）头部的长度是13位。
indexlength=3：表示音频的访问单元索引（AU-Index）的长度是3位。
indexdeltalength=3：表示音频的访问单元索引差值（AU-Index-delta）的长度是3位。
         */

        /*
         * packetization-mode
            0：单 NALU 模式。视频流中的每个 NALU 都被分配到单独的 RTP 包中。
            1：分片模式。视频流一个NALU会切割分到不同的rtp包中
         * */
        /* "m=video 0 RTP/AVP 96\r\n"
                       "a=rtpmap:96 H264/90000\r\n"
                       "a=fmtp:96 packetization-mode=1;sprop-parameter-sets=%s;profile-level-id=%s;\r\n"
                       "a=control:video\r\n",*/
        /*"m=audio 0 RTP/AVP 97\r\n"
                "a=rtpmap:97 mpeg4-generic/%s/%s\r\n"
                "a=fmtp:97 SizeLength=13;IndexLength=3;IndexDeltaLength=3;config=%s\r\n"
                "a=control:audio\r\n",*/
        /*sprintf(sdp,
                "v=0\r\n"
                "o=- 9%llu 1 IN IP4 %s\r\n"
                "t=0 0\r\n"
                "a=control:*\r\n"
                "m=video 0 RTP/AVP 96\r\n"
                "a=rtpmap:96 H264/90000\r\n"
                "a=fmtp:96 packetization-mode=1;sprop-parameter-sets=%s;profile-level-id=%s\r\n"
                "a=control:video\r\n"
                "m=audio 0 RTP/AVP 97\r\n"
                "a=rtpmap:97 mpeg4-generic/%s/%s\r\n"
                "a=fmtp:97 SizeLength=13;IndexLength=3;IndexDeltaLength=3;config=%s\r\n"
                "a=control:audio\r\n",
                time(nullptr), clientIp,
                sprop_parameter_sets.c_str(), profileLevelId.c_str(),
                std::to_string(header.sample_rate).c_str(), std::to_string(header.channel_configuration).c_str(),
                aacConfig.c_str()
        );*/
        /*上面这里使用从推流端传过来的信息，在sdpInfo里，然后返回给拉流端*/
        /*%zu 为无符号整型*/
        sprintf(responseBuffer,
                "RTSP/1.0 200 OK\r\n"
                "CSeq: %s\r\n"
                "Content-Base: %s\r\n"
                "Content-type: application/sdp\r\n"
                "Content-length: %zu\r\n"
                "\r\n"
                "%s",
                obj["CSeq"].c_str(),
                url,
                strlen(sdp),
                sdp
        );
        ret = TcpSocket::sendData(clientSocket, reinterpret_cast<uint8_t *>(responseBuffer),
                                  static_cast<int>(strlen(responseBuffer)));
        if (ret < 0) {
            fprintf(stderr, "发送数据失败 -> option\n");
            return ret;
        }
    } else if (strcmp(method, "ANNOUNCE") == 0) {

        if (obj.count("Content-Length") > 0) {
            int size = std::stoi(obj["Content-Length"]);

            while (true) {
                if (packet.length() >= size) {
                    std::string sdp = packet.substr(0, size);
                    packet.erase(0, size);

                    ret = parseSdp(sdp);
                    if (ret < 0) {
                        return 0;
                    }
                    sprintf(responseBuffer,
                            "RTSP/1.0 200 OK\r\n"
                            "CSeq: %s\r\n"
                            "Date: %s\r\n"
                            "Server: XiaoFeng\r\n"
                            "\r\n",
                            obj["CSeq"].c_str(),
                            generatorDate().c_str()
                    );
                    ret = TcpSocket::sendData(clientSocket, reinterpret_cast<uint8_t *>(responseBuffer),
                                              static_cast<int>(strlen(responseBuffer)));
                    if (ret < 0) {
                        fprintf(stderr, "发送数据失败 -> option\n");
                        return ret;
                    }
                    break;
                } else {

                    char buffer[TcpSocket::MAX_BUFFER];
                    int length = 0;
                    ret = TcpSocket::receive(clientSocket, buffer, length);
                    if (ret < 0) {
                        fprintf(stderr, "rtsp.receive failed\n");
                        return ret;
                    }
                    /*把这次读取到的追加到packet*/
                    packet.append(buffer, length);
                }
            }

        } else {
            fprintf(stderr, "ANNOUNCE 请求里没有SDP信息\n");
            return -1;
        }


    } else if (strcmp(method, "SETUP") == 0) { //客户端发送建立请求，请求建立连接会话，准备接收音视频数据

        char protocol[30]{0};
        char type[30]{0};
        char interleaved[30]{0};
        char mode[30]{0};
        num = sscanf(obj["Transport"].c_str(), "%[^;];%[^;];%[^;];%[^;]", protocol, type, interleaved, mode);
        if (num != 4) {
            fprintf(stderr, "解析Transport错误 = %s\n", obj["Transport"].c_str());
            return -1;
        }
        if (strcmp(protocol, "RTP/AVP/TCP") != 0) {
            fprintf(stderr, "只支持TCP传输\n");
            return -1;
        }

        std::string rtspUrl = url;
        size_t third_slash = rtspUrl.find('/', rtspUrl.find('/', rtspUrl.find('/') + 1) + 1);

        if (third_slash == std::string::npos) {
            fprintf(stderr, "setup , url path = null\n");
            return -1;
        }
        // 截取第三个斜杠后面的子串，就是路径部分
        std::string path = rtspUrl.substr(third_slash);

        uint8_t rtpChannel = 0;
        uint8_t rtcpChannel = 0;
        num = sscanf(interleaved, "interleaved=%hhu-%hhu", &rtpChannel, &rtcpChannel);
        if (num != 2) {
            fprintf(stderr, "解析port错误\n");
            return -1;
        }

        std::vector<std::string> pathList = split(path, "/");


        for (std::map<std::string, std::string> &map: sdpInfo.media) {
            if (map["type"] == "video" && pathList.back() == map["control"]) {
                videoChannel = rtpChannel;
            } else if (map["type"] == "audio" && pathList.back() == map["control"]) {
                audioChannel = rtpChannel;
            }

        }


        sprintf(responseBuffer,
                "RTSP/1.0 200 OK\r\n"
                "CSeq: %s\r\n"
                "Date: %s\r\n"
                "Server: XiaoFeng\r\n"
                "Transport: %s;%s;%s\r\n"
                "Session: 66334873\r\n"
                "\r\n",
                obj["CSeq"].c_str(),
                generatorDate().c_str(),
                protocol, type, interleaved
        );
        ret = TcpSocket::sendData(clientSocket, reinterpret_cast<uint8_t *>(responseBuffer),
                                  static_cast<int>(strlen(responseBuffer)));
        if (ret < 0) {
            fprintf(stderr, "发送数据失败 -> option\n");
            return ret;
        }

    } else if (strcmp(method, "RECORD") == 0) {
        sprintf(responseBuffer,
                "RTSP/1.0 200 OK\r\n"
                "CSeq: %s\r\n"
                "Date: %s\r\n"
                "Server: XiaoFeng\r\n"
                "Session: 66334873\r\n"
                "\r\n",
                obj["CSeq"].c_str(),
                generatorDate().c_str()
        );

        ret = TcpSocket::sendData(clientSocket, reinterpret_cast<uint8_t *>(responseBuffer),
                                  static_cast<int>(strlen(responseBuffer)));
        if (ret < 0) {
            fprintf(stderr, "发送数据失败 -> option\n");
            return ret;
        }

        /*这里接收客户端传过来的音视频数据，组帧*/
        ret = receiveData(packet);
        if (ret < 0) {
            fprintf(stderr, "receiveData 失败\n");
            return ret;
        }

    } else if (strcmp(method, "PLAY") == 0) { //向服务端发起播放请求
        sprintf(responseBuffer,
                "RTSP/1.0 200 OK\r\n"
                "CSeq: %s\r\n"
                "Range: npt=0.000-\r\n"
                "Session: 66334873; timeout=60\r\n"
                "\r\n",
                obj["CSeq"].c_str()
        );
        ret = TcpSocket::sendData(clientSocket, reinterpret_cast<uint8_t *>(responseBuffer),
                                  static_cast<int>(strlen(responseBuffer)));
        if (ret < 0) {
            fprintf(stderr, "发送数据失败 -> option\n");
            return ret;
        }
        videoThread = new std::thread(&Rtsp::sendVideo, this);
        audioThread = new std::thread(&Rtsp::sendAudio, this);

    } else if (strcmp(method, "TEARDOWN") == 0) {
        //结束会话请求，该请求会停止所有媒体流，并释放服务器上的相关会话数据。
        /*这里推流端和拉流端都会发送这个请求，要各自释放各自的资源*/
        sprintf(responseBuffer,
                "RTSP/1.0 200 OK\r\n"
                "CSeq: %s\r\n"
                "Date: %s\r\n"
                "Server: XiaoFeng\r\n"
                "\r\n",
                obj["CSeq"].c_str(),
                generatorDate().c_str()
        );
        ret = TcpSocket::sendData(clientSocket, reinterpret_cast<uint8_t *>(responseBuffer),
                                  static_cast<int>(strlen(responseBuffer)));
        if (ret < 0) {
            fprintf(stderr, "发送数据失败 -> option\n");
            return ret;
        }
        /*用户发送请求，结束会话，并且释放服务器资源*/
        stopFlag = false;
    } else {
        fprintf(stderr, "解析错误\n");
        return -1;
    }
    return 0;
}


int Rtsp::receiveData(std::string &packet) {


    int ret;
    /*接收rtsp数据，存为ts*/
    ts.init("test/");

    /*一个rtp包最大大小不会超过16个bit也就是65535*/
    uint8_t *buffer = new uint8_t[65535]; // NOLINT(modernize-use-auto)
    // todo delete buffer
    /*packet这里面有可能还有剩余的字节没读取完*/
    memcpy(buffer, packet.c_str(), packet.length());
    uint32_t bufferSize = packet.length();

    uint8_t *rtpBuffer = nullptr;
    uint32_t rtpBufferSize = 0;

    int size = 0;
    while (true) {
        if (bufferSize < 4) {
            ret = TcpSocket::receive(clientSocket, (char *) buffer + bufferSize, size);
            if (ret < 0) {
                fprintf(stderr, "TcpSocket::receive 失败\n");
                return ret;
            }
            bufferSize += size;
            /*这里跳过这个循环，因为有可能，还取不到四个字节数据*/
            continue;
        }
        uint8_t magic = buffer[0];
        if (magic != '$') {
            fprintf(stderr, "读取错误，没找到分隔符\n");
            return -1;
        }
        uint8_t channel = buffer[1];
        /*这个rtp包剩余数据大小*/
        uint16_t length = buffer[2] << 8 | buffer[3];

        rtpBuffer = buffer + 4;
        rtpBufferSize = bufferSize - 4;
        while (true) {
            if (rtpBufferSize < length) {
                /*不够rtp包大小继续取数据，直到取到为止*/
                ret = TcpSocket::receive(clientSocket, (char *) (rtpBuffer + rtpBufferSize), size);
                if (ret < 0) {
                    fprintf(stderr, "TcpSocket::receive 失败\n");
                    return ret;
                }
                rtpBufferSize += size;
            } else {
                /*处理数据*/
                ret = disposeRtpData(rtpBuffer, rtpBufferSize, channel, length);
                if (ret < 0) {
                    fprintf(stderr, "处理rtp包失败\n");
                    return ret;
                }
                rtpBufferSize -= length;
                /*这里不用使用memmove,不会发生内存重叠*/
                memcpy(buffer, rtpBuffer + length, rtpBufferSize);
                bufferSize = rtpBufferSize;
                break;
            }
        }
    }


    return 0;
}

static constexpr uint8_t startCode[4] = {0, 0, 0, 1};

int Rtsp::disposeRtpData(uint8_t *rtpBuffer, uint32_t rtpBufferSize, uint8_t channel, uint16_t length) {
    int ret;
    if (channel == videoChannel) {
        ReadStream rs(rtpBuffer, rtpBufferSize);
        /*获取rtp header*/
        ret = getRtpHeader(rs);
        if (ret < 0) {
            return ret;
        }
        length -= 12;

        uint8_t forbidden_zero_bit = rs.readMultiBit(1);
        uint8_t nal_ref_idc = rs.readMultiBit(2);
        uint8_t nal_unit_type = rs.readMultiBit(5);
        length -= 1;
        if (nal_unit_type == STAP_A) {
            printf("STAP_A\n");
            return -1;
        } else if (nal_unit_type == STAP_B) {
            printf("STAP_B\n");
            return -1;
        } else if (nal_unit_type == MTAP_16) {
            printf("MTAP_16\n");
            return -1;
        } else if (nal_unit_type == MTAP_24) {
            printf("MTAP_24\n");
            return -1;
        } else if (nal_unit_type == FU_A) {
            /**/
            uint8_t start = rs.readMultiBit(1);
            uint8_t end = rs.readMultiBit(1);
            /*reserved = 0*/
            rs.readMultiBit(1);

            nal_unit_type = rs.readMultiBit(5);
            length -= 1;


            if (start == 1 && end == 0) {
                *(rs.currentPtr - 1) = forbidden_zero_bit << 7 | (nal_ref_idc << 5) | nal_unit_type;
                /*这里手动给nalu加上起始码，固定0001*/
                memcpy(nalUintData, startCode, 4);
                memcpy(nalUintData + 4, rs.currentPtr - 1, length + 1);
                nalUintOffset = 4 + length + 1;
            } else if (start == 0 && end == 0) {
                memcpy(nalUintData + nalUintOffset, rs.currentPtr, length);
                nalUintOffset += length;
            } else if (start == 0 && end == 1) {
                memcpy(nalUintData + nalUintOffset, rs.currentPtr, length);
                nalUintOffset += length;

                ret = nalReader.test1(frame, nalUintData, nalUintOffset);
                if (ret < 0) {
                    fprintf(stderr, "nalReader.getPicture 失败\n");
                    return -1;
                }
                if (nalReader.pictureFinishFlag) {
                    printf("video duration = %f,idr = %d\n", frame->duration, frame->sliceHeader.nalu.IdrPicFlag);
                    /*获取到完整的一帧,做对应操作*/
                    ret = ts.writeVideo(frame);
                    if (ret < 0) {
                        fprintf(stderr, "ts.writeVideo 失败\n");
                        return -1;
                    }
                }
            } else {
                fprintf(stderr, "start = %d 和 end = %d 有问题\n", start, end);
                return -1;
            }
        } else if (nal_unit_type == FU_B) {
            printf("FU_B\n");
            return -1;
        } else {
            /*这里手动给nalu加上起始码，固定0001*/
            memcpy(rs.currentPtr - 5, startCode, 4);

            *(rs.currentPtr - 1) = forbidden_zero_bit << 7 | (nal_ref_idc << 5) | nal_unit_type;


            ret = nalReader.test1(frame, rs.currentPtr - 5, length + 5);
            if (ret < 0) {
                fprintf(stderr, "nalReader.getPicture 失败\n");
                return -1;
            }
            if (nalReader.pictureFinishFlag) {
                printf("video duration = %f,idr = %d\n", frame->duration, frame->sliceHeader.nalu.IdrPicFlag);
                /*获取到完整的一帧,做对应操作*/
                ret = ts.writeVideo(frame);
                if (ret < 0) {
                    fprintf(stderr, "ts.writeVideo 失败\n");
                    return -1;
                }
            }
//            ret = muxTransportStream(channel, rs.currentPtr - 1, length + 1);
//            if (ret < 0) {
//                fprintf(stderr, "muxTransportStream 失败\n");
//                return -1;
//            }
//            printf("nal_unit_type = %d\n", nal_unit_type);
            /*这里都认为是一个rtp包一个nalu*/
//            fs.write(reinterpret_cast<const char *>(startCode), 4);
//            *(rs.currentPtr - 1) = forbidden_zero_bit << 7 | (nal_ref_idc << 5) | nal_unit_type;
//            fs.write(reinterpret_cast<const char *>(rs.currentPtr - 1), length + 1);

        }
    } else if (channel == audioChannel) {
        //  printf("audiochanggel\n");
        ReadStream rs(rtpBuffer, rtpBufferSize);
        /*获取rtp header*/
        ret = getRtpHeader(rs);
        if (ret < 0) {
            return ret;
        }


        uint16_t headerLength = rs.readMultiBit(16) / 16;
        int size = 0;
        int offset = 0;
        uint8_t *ptr = rs.currentPtr;

        std::vector<uint16_t> sizeList;
        for (int i = 0; i < headerLength; ++i) {
            uint16_t sizeLength = rs.readMultiBit(13);
            uint8_t indexDelta = rs.readMultiBit(3);
            sizeList.push_back(sizeLength);


//            WriteStream ws(adtsHeader, 7);
//            header.setConfig(audioObjectType - 1, samplingFrequencyIndex, channelConfiguration, sizeLength + 7);
//            header.writeAdtsHeader(ws);
//
//
//            fs.write(reinterpret_cast<const char *>(adtsHeader), 7);
//            offset = (headerLength * 2) + size;
//            fs.write(reinterpret_cast<const char *>(ptr + offset), sizeLength);
//
//            size += sizeLength;
            /*rs.setBytePtr(sizeLength);
            i*/
        }
        for (uint16_t aacSize: sizeList) {
            AdtsHeader::setFrameLength(aacSize + 7);
            memcpy(rs.currentPtr - 7, AdtsHeader::header, 7);

            adtsReader.putAACData(adtsHeader, rs.currentPtr - 7, aacSize + 7);
            // printf("audio duration = %f\n", adtsHeader.duration);

            //   muxTransportStream(channel, header.data, header.size);
            rs.setBytePtr(aacSize);

        }


    } else {
        printf("rtcp channel = %d\n", channel);
    }

    return 0;
}


/*int Rtsp::muxTransportStream(uint8_t channel, uint8_t *data, uint32_t size) {
    int ret;
    ret = nalReader.putNalUintData(frame, data, size);
    if (ret < 0) {
        fprintf(stderr, "nalReader.getPicture 失败\n");
        return -1;
    }
    if (nalReader.pictureFinishFlag) {
        *//*获取到完整的一帧,做对应操作*//*
    }

    return 0;
}*/

/*发送音频和视频的函数*/
int Rtsp::sendVideo() {
    int ret;

    RtpPacket videoPacket;
    videoPacket.init(NALReader::MAX_BUFFER_SIZE, RTP_PAYLOAD_TYPE_H264);


    bool flag = true;

    NALReader reader;
    ret = reader.init("");
    if (ret < 0) {
        fprintf(stderr, "init video failed\n");
        videoSendError = true;
        return ret;
    }
    NALPicture *picture = reader.allocPicture();
    while (flag) {
        ret = reader.getVideoFrame(picture, flag);
        if (ret < 0) {
            fprintf(stderr, "获取视频帧失败\n");
            videoSendError = true;
            return ret;
        }

        for (int i = 0; i < picture->data.size(); ++i) {
            const Frame &nalUint = picture->data[i];

            ret = videoPacket.sendVideoFrame(clientSocket, nalUint.data, nalUint.nalUintSize,
                                             i == (picture->data.size() - 1), videoChannel);
            if (ret < 0) {
                fprintf(stderr, "发送视频数据失败\n");
                videoSendError = true;
                return ret;
            }

        }
        // printf("pts = %lld\n", picture->pts);
        videoPacket.timestamp = picture->pts;


        std::this_thread::sleep_for(std::chrono::milliseconds(40 - 20));
    }
    printf("视频发送完成\n");
    return 0;
}

int Rtsp::sendAudio() {
    int ret;

    RtpPacket audioPacket;
    audioPacket.init(AdtsReader::MAX_BUFFER_SIZE, RTP_PAYLOAD_TYPE_AAC);

    AdtsReader reader;
    ret = reader.init("");
    if (ret < 0) {
        fprintf(stderr, "init audio failed\n");
        audioSendError = true;
        return ret;
    }


    AdtsHeader header;
    bool flag = true;
    while (flag) {
        ret = reader.getAudioFrame(header, flag);
        if (ret < 0) {
            fprintf(stderr, "获取audio frame失败\n");
            audioSendError = true;
            return ret;
        }
        ret = audioPacket.sendAudioPacket(clientSocket, header.data, header.size, audioChannel);
        if (ret < 0) {
            fprintf(stderr, "发送音频包失败\n");
            audioSendError = true;
            return ret;
        }

        audioPacket.timestamp += 1024;
        if (audioPacket.timestamp >= 4294967295) {
            audioPacket.timestamp = 0;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    printf("音频发送完成\n");
    return 0;
}

int Rtsp::parseSdp(const std::string &sdp) {
    int ret;
    std::vector<std::string> list = split(sdp, "\r\n");

    //std::map<std::string, std::string> obj;
    for (int i = 0; i < list.size(); ++i) {
        const std::string &line = list[i];

        std::string::size_type pos = line.find('=');
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1, line.length() - 2);
            if (key == "v") {
                sdpInfo.version = value;
            } else if (key == "o") {
                std::vector<std::string> originList = split(value, " ");
                sdpInfo.origin["username"] = originList[0];
                sdpInfo.origin["sessionId"] = originList[1];
                sdpInfo.origin["sessionVersion"] = originList[2];
                sdpInfo.origin["netType"] = originList[3];
                sdpInfo.origin["ipVer"] = originList[4];
                sdpInfo.origin["address"] = originList[5];
            } else if (key == "s") {
                /*是一个必需的字段,表示会话名称，可以是任意字符串*/
                sdpInfo.name = value;
            } else if (key == "t") {
                /*是一个必需的字段，它表示会话的时间信息。它包括两个或多个时间戳，分别表示会话的开始时间和结束时间。*/
                std::vector<std::string> timingList = split(value, " ");
                sdpInfo.timing["start"] = timingList[0];
                sdpInfo.timing["stop"] = timingList[1];
            } else if (key == "m") {
                /*遇到媒体层了*/
                ret = parseMediaLevel(i, list);
                if (ret < 0) {
                    return ret;
                }
                break;
            }


            //obj[key] = value;
        }
    }
    return 0;
}


int Rtsp::parseMediaLevel(int i, const std::vector<std::string> &list) {
    int ret;
    int num = -1;
    for (int j = i; j < list.size(); ++j) {
        const std::string &line = list[j];

        std::string::size_type pos = line.find('=');

        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            if (key == "m") {
                ++num;
                std::vector<std::string> mediaList = split(value, " ");
                sdpInfo.media[num]["type"] = mediaList[0];
                sdpInfo.media[num]["port"] = mediaList[1];
                sdpInfo.media[num]["protocol"] = mediaList[2];
                sdpInfo.media[num]["payloads"] = mediaList[3];

            } else if (key == "a") {
                std::string::size_type position = value.find(' ');
                if (position != std::string::npos) {
                    std::string left = value.substr(0, position);
                    std::string right = value.substr(position + 1);
                    if (sdpInfo.media[num]["type"] == "video" && sdpInfo.media[num]["payloads"] == "96") {
                        if (left == "rtpmap:96") {
                            sdpInfo.media[num]["rtpmap"] = right;
                        } else if (left == "fmtp:96") {
                            sdpInfo.media[num]["fmtp"] = right;
                            // fmtp:96 packetization-mode=1; sprop-parameter-sets=Z2QAH6zZQFAFuhAAAAMAEAAAAwPA8YMZYA==,aOvjyyLA; profile-level-id=64001F
                            std::map<std::string, std::string> obj = getRtspObj(split(right, ";"), "=");
                            std::vector<std::string> sps_pps = split(obj["sprop-parameter-sets"], ",");

                            uint8_t sps[50];
                            memcpy(sps, startCode, 4);
                            ret = base64_decode(sps_pps[0], sps + 4);
                            if (ret < 0) {
                                fprintf(stderr, "解析sps失败\n");
                                return ret;
                            }
//                            spsSize = ret;
                            ret = nalReader.test1(frame, sps, ret + 4);
                            if (ret < 0) {
                                fprintf(stderr, "nalReader.getPicture 失败\n");
                                return -1;
                            }

                            uint8_t pps[50];
                            memcpy(pps, startCode, 4);
                            ret = base64_decode(sps_pps[1], pps + 4);
                            if (ret < 0) {
                                fprintf(stderr, "解析pps失败\n");
                                return ret;
                            }
//                            ppsSize = ret;
                            ret = nalReader.test1(frame, pps, ret + 4);
                            if (ret < 0) {
                                fprintf(stderr, "nalReader.getPicture 失败\n");
                                return -1;
                            }
                        } else {
                            fprintf(stderr, "解析错误\n");
                            return -1;
                        }
                    } else if (sdpInfo.media[num]["type"] == "audio" && sdpInfo.media[num]["payloads"] == "97") {
                        if (left == "rtpmap:97") {
                            sdpInfo.media[num]["rtpmap"] = right;
                        } else if (left == "fmtp:97") {
//                            fmtp:97 profile-level-id=1;mode=AAC-hbr;sizelength=13;indexlength=3;indexdeltalength=3; config=119056E500
                            std::map<std::string, std::string> obj = getRtspObj(split(right, ";"), "=");
                            sdpInfo.media[num]["fmtp"] = right;
                            ret = parseAACConfig(obj["config"]);
                            if (ret < 0) {
                                return ret;
                            }
                        } else {
                            fprintf(stderr, "解析错误\n");
                            return -1;
                        }
                    }

                } else {
                    /*这里默认认为他是a=control,有可能有bug*/
                    std::vector<std::string> controlList = split(value, ":");
                    sdpInfo.media[num]["control"] = controlList[1];
                }
            }
        }
    }
    return 0;
}

std::map<std::string, std::string> Rtsp::getRtspObj(const std::vector<std::string> &list, const std::string &spacer) {
    std::map<std::string, std::string> obj;
    for (const std::string &str: list) {
        std::string::size_type pos = str.find(spacer);
        if (pos != std::string::npos) {
            std::string key = trim(str.substr(0, pos));
            std::string value = trim(str.substr(pos + 1));
            obj[key] = value;
        }
    }

    return obj;
}


int Rtsp::parseAACConfig(const std::string &config) {
    // 把十六进制字符串转换成二进制字符串
    std::string bin;
    for (char i: config) {
        switch (i) {
            case '0':
                bin.append("0000");
                break;
            case '1':
                bin.append("0001");
                break;
            case '2':
                bin.append("0010");
                break;
            case '3':
                bin.append("0011");
                break;
            case '4':
                bin.append("0100");
                break;
            case '5':
                bin.append("0101");
                break;
            case '6':
                bin.append("0110");
                break;
            case '7':
                bin.append("0111");
                break;
            case '8':
                bin.append("1000");
                break;
            case '9':
                bin.append("1001");
                break;
            case 'A':
                bin.append("1010");
                break;
            case 'B':
                bin.append("1011");
                break;
            case 'C':
                bin.append("1100");
                break;
            case 'D':
                bin.append("1101");
                break;
            case 'E':
                bin.append("1110");
                break;
            case 'F':
                bin.append("1111");
                break;
            default:
                fprintf(stderr, "输入的十六进制有问题\n");
                return -1;
        }
    }

    // 按照MPEG-4音频标准中的规则，把二进制字符串分成以下几个字段，并输出它们的值
    uint8_t audioObjectType = std::stoi(bin.substr(0, 5), nullptr, 2); // 前5位，表示音频对象类型（Audio Object Type）
    uint8_t samplingFrequencyIndex = std::stoi(bin.substr(5, 4), nullptr, 2); // 后4位，表示采样率索引（Sampling Frequency Index）
    uint8_t channelConfiguration = std::stoi(bin.substr(9, 4), nullptr, 2); // 后4位，表示声道配置（Channel Configuration）
    uint8_t GASpecificConfig = std::stoi(bin.substr(13, 3), nullptr, 2); // 后3位，表示全局音频特定配置信息

    /*写入adts header*/
    WriteStream ws(AdtsHeader::header, 7);
    adtsHeader.setConfig(audioObjectType - 1, samplingFrequencyIndex, channelConfiguration, 0);
    adtsHeader.writeAdtsHeader(ws);

    return 0;
}


Rtsp::~Rtsp() {
    if (videoThread && videoThread->joinable()) {
        videoThread->join();
        delete videoThread;
    }

    if (audioThread && audioThread->joinable()) {
        audioThread->join();
        delete audioThread;
    }
}








