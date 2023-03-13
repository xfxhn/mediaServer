
#include "AnalysisData.h"
#include <ctime>

#include "util.h"


char *AnalysisData::getLine(char *buf, char *line) {
    while (*buf != '\n') {
        *line = *buf;
        line++;
        buf++;
    }

    *line = '\n';
    ++line;
    *line = '\0';

    ++buf;
    return buf;
}


int AnalysisData::getInfo(TcpSocket &rtsp, const char *requestBuffer) {
    int ret;

    char method[20]{0};
    char url[50]{0};
    char version[20]{0};
    //  char line[512]{0};
    char responseBuffer[1024]{0};

    int num = 0;
    std::vector<std::string> list = split(requestBuffer, "\r\n");
    //list.front()
    // char *bufPtr = getLine(requestBuffer, line);
    num = sscanf(list.front().c_str(), "%s %s %s\r\n", method, url, version);
    if (num != 3) {
        fprintf(stderr, "解析method, url, version失败\n");
        return -1;
    }
    //list.pop_front();
    list.erase(list.begin());

    printf("client->server        method = %s\n", method);

    std::map<std::string, std::string> obj = getRtspObj(list);

    if (strcmp(method, "OPTIONS") == 0) {//获取服务端提供的可用方法
        /*回复客户端当前可用的方法*/
        sprintf(responseBuffer,
                "RTSP/1.0 200 OK\r\n"
                "CSeq: %s\r\n"
                "Public: OPTIONS, DESCRIBE, SETUP, TEARDOWN, PLAY, PAUSE, ANNOUNCE, RECORD, SET_PARAMETER, GET_PARAMETER\r\n"
                "Server: XiaoFeng\r\n"
                "\r\n",
                obj["CSeq"].c_str()
        );

        ret = rtsp.sendData(reinterpret_cast<uint8_t *>(responseBuffer), static_cast<int>(strlen(responseBuffer)));
        if (ret < 0) {
            fprintf(stderr, "发送数据失败 -> option\n");
            return ret;
        }

    } else if (strcmp(method, "ANNOUNCE") == 0) {

    } else if (strcmp(method, "SETUP") == 0) { //客户端发送建立请求，请求建立连接会话，准备接收音视频数据

        char protocol[30]{0};
        char type[30]{0};
        char interleaved[30]{0};
        num = sscanf(obj["Transport"].c_str(), "%[^;];%[^;];%s", protocol, type, interleaved);
        if (num != 3) {
            fprintf(stderr, "解析Transport错误\n");
            return -1;
        }
        if (strcmp(protocol, "RTP/AVP/TCP") != 0) {
            fprintf(stderr, "只支持TCP传输\n");
            return -1;
        }

        uint8_t rtpChannel = 0;
        uint8_t rtcpChannel = 0;
        num = sscanf(interleaved, "interleaved=%hhu-%hhu", &rtpChannel, &rtcpChannel);
        if (num != 2) {
            fprintf(stderr, "解析port错误\n");
            return -1;
        }
        if (strcmp(url, "rtsp://127.0.0.1:8554/audio") == 0) {
            audioChannel = rtpChannel;
        } else if (strcmp(url, "rtsp://127.0.0.1:8554/video") == 0) {
            videoChannel = rtpChannel;
        } else {
            fprintf(stderr, "客户端发送过来的链接不对 %s\n", url);
            return -1;
        }


        sprintf(responseBuffer,
                "RTSP/1.0 200 OK\r\n"
                "CSeq: %s\r\n"
                "Transport: %s;%s;%s\r\n"
                "Session: 66334873\r\n"
                "\r\n",
                obj["CSeq"].c_str(),
                protocol, type, interleaved
        );
        ret = rtsp.sendData(reinterpret_cast<uint8_t *>(responseBuffer), static_cast<int>(strlen(responseBuffer)));
        if (ret < 0) {
            fprintf(stderr, "发送数据失败 -> option\n");
            return ret;
        }

    } else if (strcmp(method, "DESCRIBE") == 0) { //向服务端获取对应会话的媒体描述信息
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




        /*在 SDP 中，fmtp 参数通常与 rtpmap 参数一起使用，rtpmap 用于指定媒体流的编码类型和格式，
         * 而 fmtp 用于指定编码器的具体配置和参数，例如帧率、分辨率、码率、I 帧间隔等。
         * 这些参数可以根据需要进行调整，以获得最佳的音视频质量和传输效率。*/
        /*
         * 如果FMTP中的SizeLength参数设置为13，那么它指定了每个NAL单元头部中长度字段的长度为13个比特。
         这意味着，每个NAL单元头部中包含的长度信息将用13位二进制数表示，可以表示的最大值为8191（2的13次方减1）。
        这个具体的设置可能是针对某种特定的视频编码标准或压缩方案而设定的，
         因为不同的编码标准和方案可能需要不同的位数来表示NAL单元的长度信息。
         因此，通过在FMTP中指定SizeLength参数，可以确保解码器正确地解析每个NAL单元，
         并恢复出正确的视频帧数据，以便实现正确的视频播放*/



        /*
 sizelength=13;indexlength=3;indexdeltalength=3
涉及到AAC的AU Header,如果一个RTP包则只包含一个AAC包，则没有这三个字段,有则说明是含有AU Header字段，具体参考RTP对音频AAC的封装。
AU-size由sizeLength决定表示本段音频数据占用的字节数
AU-Index由indexLength决定表示本段的序号, 通常0开始
AU-Index-delta由indexdeltaLength 决定表示本段序号与上一段序号的差值
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
        sprintf(sdp,
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
        );
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
        ret = rtsp.sendData(reinterpret_cast<uint8_t *>(responseBuffer), static_cast<int>(strlen(responseBuffer)));
        if (ret < 0) {
            fprintf(stderr, "发送数据失败 -> option\n");
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
        ret = rtsp.sendData(reinterpret_cast<uint8_t *>(responseBuffer), static_cast<int>(strlen(responseBuffer)));
        if (ret < 0) {
            fprintf(stderr, "发送数据失败 -> option\n");
            return ret;
        }
        return 2;
    } else if (strcmp(method, "TEARDOWN") == 0) { //结束会话请求，该请求会停止所有媒体流，并释放服务器上的相关会话数据。
        sprintf(responseBuffer,
                "RTSP/1.0 200 OK\r\n"
                "CSeq: %s\r\n"
                "\r\n",
                obj["CSeq"].c_str()
        );
        ret = rtsp.sendData(reinterpret_cast<uint8_t *>(responseBuffer), static_cast<int>(strlen(responseBuffer)));
        if (ret < 0) {
            fprintf(stderr, "发送数据失败 -> option\n");
            return ret;
        }
        return 1;
    } else {
        fprintf(stderr, "解析错误\n");
        return -1;
    }
    return 0;
}


std::map<std::string, std::string> AnalysisData::getRtspObj(std::vector<std::string> &list) {
    std::map<std::string, std::string> obj;
    for (const std::string &str: list) {
        std::string::size_type pos = str.find(':');
        if (pos != std::string::npos) {
            std::string key = str.substr(0, pos);
            std::string value = str.substr(pos + 2, str.length() - key.length() - 2);
            obj[key] = value;
        }
    }

    return obj;
}

/*parseRtsp*/
int AnalysisData::test1(TcpSocket &rtsp, Task *task) {

}

int AnalysisData::test(TcpSocket &rtsp) {
    int ret;
    char buffer[TcpSocket::MAX_BUFFER]{0};
    int length = 0;
    std::string packet;
    while (true) {
        /*dataSize本次获取的数据大小*/
        ret = rtsp.receive(buffer, length);
        if (ret < 0) {
            fprintf(stderr, "rtsp.receive failed\n");
            return ret;
        }

        // 将缓冲区中的数据添加到字符串对象中
        packet.append(buffer, length);
        // 查找分隔符在字符串中的位置
        size_t pos = packet.find("\r\n\r\n");
        while (pos != std::string::npos) {
            // 截取出一个完整的数据包（不含分隔符）
            std::string data = packet.substr(0, pos);
            // 打印出数据包内容
//            std::cout << data << std::endl;
            // 去掉已处理的部分（含分隔符）
            packet.erase(0, pos + 4);


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
            // list.pop_front();
            list.erase(list.begin());

            std::map<std::string, std::string> obj = getRtspObj(list);

            if (strcmp(method, "OPTIONS") == 0) {
                /*回复客户端当前可用的方法*/
                sprintf(responseBuffer,
                        "RTSP/1.0 200 OK\r\n"
                        "CSeq: %s\r\n"
                        "Public: OPTIONS, DESCRIBE, SETUP, TEARDOWN, PLAY, PAUSE, ANNOUNCE, RECORD, SET_PARAMETER, GET_PARAMETER\r\n"
                        "Server: XiaoFeng\r\n"
                        "\r\n",
                        obj["CSeq"].c_str()
                );

                ret = rtsp.sendData(reinterpret_cast<uint8_t *>(responseBuffer),
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
                            ret = rtsp.sendData(reinterpret_cast<uint8_t *>(responseBuffer),
                                                static_cast<int>(strlen(responseBuffer)));
                            if (ret < 0) {
                                fprintf(stderr, "发送数据失败 -> option\n");
                                return ret;
                            }
                            break;
                        } else {
                            ret = rtsp.receive(buffer, length);
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
                num = sscanf(obj["Transport"].c_str(), "%[^;];%[^;];%s", protocol, type, interleaved);
                if (num != 3) {
                    fprintf(stderr, "解析Transport错误\n");
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
                    } else {
                        fprintf(stderr, "找不到对应的control\n");
                        return -1;
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
                ret = rtsp.sendData(reinterpret_cast<uint8_t *>(responseBuffer),
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

                ret = rtsp.sendData(reinterpret_cast<uint8_t *>(responseBuffer),
                                    static_cast<int>(strlen(responseBuffer)));
                if (ret < 0) {
                    fprintf(stderr, "发送数据失败 -> option\n");
                    return ret;
                }

            }



            // 继续查找下一个分隔符位置
            pos = packet.find("\r\n\r\n");
        }
    }

    return 0;
}

int AnalysisData::receiveRtspData(TcpSocket &rtsp) {
    int ret = 0;


    char *data = nullptr;
    int dataSize = 0;

    char buf[TcpSocket::MAX_BUFFER]{0};
    int size = 0;

    /*rtsp 最后一行则直接为回车符（CR）和换行符（LF），表示本次请求报文结束*/
    while (true) {
        /*dataSize本次获取的数据大小*/
        ret = rtsp.receive(buf, dataSize);
        if (ret < 0) {
            fprintf(stderr, "rtsp.receive failed\n");
            return ret;
        }
//        int bufferSize = recv(clientSocket, buf, MAX_BUFFER, 0);
//        /*这里返回0的话表示对端关闭了连接，发送了FIN包*/
//        if (bufferSize <= 0) {
//            ret = bufferSize;
//            break;
//        }
        char *temp = new char[size + dataSize + 1];
        /*先把data的数据全部拷贝到temp上*/
        memcpy(temp, data, size);
        /*再把新接收的数据拷贝到temp上*/
        memcpy(temp + size, buf, dataSize);

        delete[] data;
        data = temp;

        size += dataSize;

        if (data[size - 1] == '\n' && data[size - 2] == '\r' && data[size - 3] == '\n' && data[size - 4] == '\r') {
            /*表示这次请求报文结束*/
            data[size] = '\0';


            getInfo(rtsp, data);
            ret = 1;
            break;
        }
    }

    //dataSize = size;
    return ret;
}

int AnalysisData::receiveResidualData(int size) {
    int ret;
    char buf[TcpSocket::MAX_BUFFER]{0};

    int dataSize = 0;
    //  ret = rtsp.receive(buf, dataSize);


    return 0;
}

int AnalysisData::parseSdp(const std::string &sdp) {
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

int AnalysisData::parseMediaLevel(int i, const std::vector<std::string> &list) {

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
                        } else {
                            fprintf(stderr, "解析错误\n");
                            return -1;
                        }
                    } else if (sdpInfo.media[num]["type"] == "audio" && sdpInfo.media[num]["payloads"] == "97") {
                        if (left == "rtpmap:97") {
                            sdpInfo.media[num]["rtpmap"] = right;
                        } else if (left == "fmtp:97") {
                            sdpInfo.media[num]["fmtp"] = right;
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


