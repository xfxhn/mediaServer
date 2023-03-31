
#include "rtsp.h"
#include <filesystem>
#include <chrono>
#include "util.h"
#include "parseUrl.h"

#include "rtspReceiveData.h"
#include "readStream.h"
#include "writeStream.h"


#define RTP_PAYLOAD_TYPE_H264   96
#define RTP_PAYLOAD_TYPE_AAC    97


static std::map<std::string, Info> flowInfo;
/*当前写到第几个ts包了*/
//uint32_t transportStreamPacketNumber{0};
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
    //nalUintData = new uint8_t[NALReader::MAX_BUFFER_SIZE];

    // nalReader.init1();

//    aacFrameData = new uint8_t[AdtsReader::MAX_BUFFER_SIZE];
//    aacHeader = aacFrameData;
//    aacData = &aacFrameData[7];

//    frame = nalReader.allocPicture();
//    if (frame == nullptr) {
//        fprintf(stderr, "获取frame失败\n");
//        return -1;
//    }
    /*fs.open("resource/bbbbbb.aac", std::ios::binary | std::ios::out | std::ios::trunc);
    if (!fs.is_open()) {
        fprintf(stderr, "cloud not open %s\n", "resource/bbbbbb.aac");
        return -1;
    }*/
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
    char protocolVersion[20]{0};

    char responseBuffer[1024]{0};


    int num = sscanf(list.front().c_str(), "%s %s %s\r\n", method, url, protocolVersion);
    if (num != 3) {
        fprintf(stderr, "解析method, url, version失败\n");
        return -1;
    }
    ParseUrl urlUtils(url, 554);
    urlUtils.parse();

    list.erase(list.begin());

    std::map<std::string, std::string> obj = getObj(list, ":");

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

        //  std::string session;
        /*这个语法是C++ 17的*/
        for (auto &[key, value]: flowInfo) {
            if (value.dir == urlUtils.getPath()) {
                pullSession = value.session;
                dir = value.dir;
                break;
            }
        }
        if (pullSession.empty()) {
            sprintf(responseBuffer,
                    "RTSP/1.0 404 找不到流\r\n"
                    "CSeq: %s\r\n"
                    "Date: %s\r\n"
                    "Server: XiaoFeng\r\n"
                    "Connection: Close\r\n"
                    "\r\n",
                    obj["CSeq"].c_str(),
                    generatorDate().c_str()
            );
            ret = TcpSocket::sendData(clientSocket, reinterpret_cast<uint8_t *>(responseBuffer),
                                      static_cast<int>(strlen(responseBuffer)));
            if (ret < 0) {
                fprintf(stderr, "发送数据失败\n");
                return ret;
            }
            return -1;
        }
        SdpInfo &sdpInfo = flowInfo[pullSession].sdp;


        /* todo 这里可以根据写入的流来自己生成sdp的一些信息 */

        std::string videoFmtp;
        std::string videoRtpmap;
        std::string audioFmtp;
        std::string audioRtpmap;

        for (std::map<std::string, std::string> &map: sdpInfo.media) {
            if (map["type"] == "video" && map.count("fmtp") && map.count("rtpmap")) {
                videoFmtp = map["fmtp"];
                videoRtpmap = map["rtpmap"];
                videoControl = map["control"];
            } else if (map["type"] == "audio" && map.count("fmtp") && map.count("rtpmap")) {
                audioFmtp = map["fmtp"];
                audioRtpmap = map["rtpmap"];
                audioControl = map["control"];
            } else {
                fprintf(stderr, "没找到对应的fmtp和rtpmap\n");
                return -1;
            }
        }

        sprintf(sdp,
                "v=0\r\n"
                "o=- 9%llu 0 IN IP4 %s\r\n"
                "t=0 0\r\n"
                "a=control:*\r\n"
                "m=video 0 RTP/AVP 96\r\n"
                "a=rtpmap:96 %s\r\n"
                "a=fmtp:96 %s\r\n"
                "a=control:%s\r\n"
                "m=audio 0 RTP/AVP 97\r\n"
                "a=rtpmap:97 %s\r\n"
                "a=fmtp:97 %s\r\n"
                "a=control:%s\r\n",
                time(nullptr), urlUtils.getDomain().c_str(),
                videoRtpmap.c_str(),
                videoFmtp.c_str(),
                videoControl.c_str(),
                audioRtpmap.c_str(),
                audioFmtp.c_str(),
                audioControl.c_str()
        );
        /*给每个拉流的生成一个唯一id*/
        uniqueSession = generate_unique_string();
        /*上面这里使用从推流端传过来的信息，在sdpInfo里，然后返回给拉流端*/
        /*%zu 为无符号整型*/
        sprintf(responseBuffer,
                "RTSP/1.0 200 OK\r\n"
                "CSeq: %s\r\n"
                "Content-Base: %s\r\n"
                "Content-type: application/sdp\r\n"
                "Content-length: %zu\r\n"
                "Session: %s\r\n"
                "\r\n"
                "%s",
                obj["CSeq"].c_str(),
                url,
                strlen(sdp),
                uniqueSession.c_str(),
                sdp
        );
        ret = TcpSocket::sendData(clientSocket, reinterpret_cast<uint8_t *>(responseBuffer),
                                  static_cast<int>(strlen(responseBuffer)));
        if (ret < 0) {
            fprintf(stderr, "发送数据失败\n");
            return ret;
        }
    } else if (strcmp(method, "ANNOUNCE") == 0) {

        for (std::pair<const std::string, Info> &item: flowInfo) {
            if (item.second.dir == urlUtils.getPath()) {
                sprintf(responseBuffer,
                        "RTSP/1.0 500 换个路径\r\n"
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
                    fprintf(stderr, "发送数据失败\n");
                    return ret;
                }
                /*这里直接退出，并且关闭连接，也可以继续等待客户端重新发送请求*/
                return -1;
            }
        }


        if (obj.count("Content-Length") > 0) {
            int size = std::stoi(obj["Content-Length"]);

            while (true) {
                if (packet.length() >= size) {
                    std::string sdp = packet.substr(0, size);
                    packet.erase(0, size);


                    /*推流上来会生成一个唯一的session*/
                    uniqueSession = generate_unique_string();

                    Info info;
                    info.session = uniqueSession;
                    info.dir = urlUtils.getPath();
                    ret = parseSdp(sdp, info.sdp);
                    if (ret < 0) {
                        sprintf(responseBuffer,
                                "RTSP/1.0 500 解析SDP失败\r\n"
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
                            fprintf(stderr, "发送数据失败\n");
                            return ret;
                        }
                        return 0;
                    }
                    for (auto &media: info.sdp.media) {
                        if (media["type"] == "video") {
                            videoControl = media["control"];
                        } else if (media["type"] == "audio") {
                            audioControl = media["control"];
                        }
                    }
                    flowInfo[uniqueSession] = info;

                    sprintf(responseBuffer,
                            "RTSP/1.0 200 OK\r\n"
                            "CSeq: %s\r\n"
                            "Date: %s\r\n"
                            "Server: XiaoFeng\r\n"
                            "Session: %s\r\n"
                            "\r\n",
                            obj["CSeq"].c_str(),
                            generatorDate().c_str(),
                            uniqueSession.c_str()
                    );
                    ret = TcpSocket::sendData(clientSocket, reinterpret_cast<uint8_t *>(responseBuffer),
                                              static_cast<int>(strlen(responseBuffer)));
                    if (ret < 0) {
                        fprintf(stderr, "发送数据失败\n");
                        return ret;
                    }
                    break;
                } else {

                    char buffer[TcpSocket::MAX_BUFFER];
                    int length = 0;
                    ret = TcpSocket::receive(clientSocket, buffer, length);
                    if (ret < 0) {
                        fprintf(stderr, "rtsp.receive failed\n");;
                        return ret;
                    }
                    /*把这次读取到的追加到packet*/
                    packet.append(buffer, length);
                }
            }

        } else {
            fprintf(stderr, "ANNOUNCE 请求里没有SDP信息\n");
            sprintf(responseBuffer,
                    "RTSP/1.0 451 参数有问题\r\n"
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
                fprintf(stderr, "发送数据失败\n");
                return ret;
            }
            return -1;
        }


    } else if (strcmp(method, "SETUP") == 0) { //客户端发送建立请求，请求建立连接会话，准备接收音视频数据

        char protocol[30]{0};
        char type[30]{0};
        char interleaved[30]{0};
        /*char mode[30]{ 0 };*/
        num = sscanf(obj["Transport"].c_str(), "%[^;];%[^;];%[^;]", protocol, type, interleaved);
        if (num != 3) {
            fprintf(stderr, "解析Transport错误 = %s\n", obj["Transport"].c_str());
            sprintf(responseBuffer,
                    "RTSP/1.0 451 解析Transport错误\r\n"
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
                fprintf(stderr, "发送数据失败\n");
                return ret;
            }
            return -1;
        }
        if (strcmp(protocol, "RTP/AVP/TCP") != 0) {
            fprintf(stderr, "只支持TCP传输\n");
            sprintf(responseBuffer,
                    "RTSP/1.0 501 只支持TCP传输\r\n"
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
                fprintf(stderr, "发送数据失败\n");
                return ret;
            }
            return -1;
        }


        uint8_t rtpChannel = 0;
        uint8_t rtcpChannel = 0;
        num = sscanf(interleaved, "interleaved=%hhu-%hhu", &rtpChannel, &rtcpChannel);
        if (num != 2) {
            fprintf(stderr, "解析port错误  %s\n", interleaved);
            sprintf(responseBuffer,
                    "RTSP/1.0 451 解析port错误\r\n"
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
                fprintf(stderr, "发送数据失败\n");
                return ret;
            }
            return -1;
        }

        // 截取第三个斜杠后面的子串，就是路径部分
        std::string path = urlUtils.getPath();
        std::vector<std::string> pathList = split(path, "/");

        /*如果是推流，rtpChannel 表示推流上来的数据是音频还是视频，推流就用ANNOUNCE请求里的control字段来判断当前SETUP请求里rtpChannel是音频还是视频*/
        /*如果是拉流，rtpChannel 表示拉流的数据是音频还是视频，拉流就用我发送给客户端的DESCRIBE请求里的control字段来判断当前SETUP请求里rtpChannel是音频还是视频*/
        if (pathList.back() == videoControl) {
            videoChannel = rtpChannel;
        } else if (pathList.back() == audioControl) {
            audioChannel = rtpChannel;
        } else {
            fprintf(stderr, "没找到对应的流 -- %s\n", pathList.back().c_str());
            sprintf(responseBuffer,
                    "RTSP/1.0 500 没找到对应的流\r\n"
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
                fprintf(stderr, "发送数据失败\n");
                return ret;
            }
            return -1;
        }


        sprintf(responseBuffer,
                "RTSP/1.0 200 OK\r\n"
                "CSeq: %s\r\n"
                "Date: %s\r\n"
                "Server: XiaoFeng\r\n"
                "Transport: %s;%s;%s\r\n"
                "Session: %s\r\n"
                "\r\n",
                obj["CSeq"].c_str(),
                generatorDate().c_str(),
                protocol, type, interleaved,
                obj["Session"].c_str()
        );
        ret = TcpSocket::sendData(clientSocket, reinterpret_cast<uint8_t *>(responseBuffer),
                                  static_cast<int>(strlen(responseBuffer)));
        if (ret < 0) {
            fprintf(stderr, "发送数据失败\n");

            return ret;
        }

    } else if (strcmp(method, "RECORD") == 0) {
        sprintf(responseBuffer,
                "RTSP/1.0 200 OK\r\n"
                "CSeq: %s\r\n"
                "Date: %s\r\n"
                "Server: XiaoFeng\r\n"
                "Session: %s\r\n"
                "\r\n",
                obj["CSeq"].c_str(),
                generatorDate().c_str(),
                obj["Session"].c_str()
        );

        ret = TcpSocket::sendData(clientSocket, reinterpret_cast<uint8_t *>(responseBuffer),
                                  static_cast<int>(strlen(responseBuffer)));
        if (ret < 0) {
            fprintf(stderr, "发送数据失败 -> option\n");
            return ret;
        }

        /*这里接收客户端传过来的音视频数据，组帧*/
        ret = receiveData(packet, obj["Session"]);
        if (ret < 0) {
            fprintf(stderr, "receiveData 失败\n");
            memset(responseBuffer, 0, 1024);
            sprintf(responseBuffer,
                    "RTSP/1.0 500 接收音视频数据失败\r\n"
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
                fprintf(stderr, "发送数据失败\n");
                return ret;
            }
            return ret;
        }

    } else if (strcmp(method, "PLAY") == 0) { //向服务端发起播放请求

        int transportStreamPacketNumber = flowInfo[pullSession].transportStreamPacketNumber;

        if (transportStreamPacketNumber - 1 < 0) {
            fprintf(stderr, "还没准备好，等一会儿在拉流\n");
            sprintf(responseBuffer,
                    "RTSP/1.0 500 还没准备好，等一会儿在拉流\r\n"
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
                fprintf(stderr, "发送数据失败\n");
                return ret;
            }
            return -1;
        }

        sprintf(responseBuffer,
                "RTSP/1.0 200 OK\r\n"
                "CSeq: %s\r\n"
                "Range: npt=0.000-\r\n"
                "Session: %s; timeout=60\r\n"
                "\r\n",
                obj["CSeq"].c_str(),
                obj["Session"].c_str()
        );
        ret = TcpSocket::sendData(clientSocket, reinterpret_cast<uint8_t *>(responseBuffer),
                                  static_cast<int>(strlen(responseBuffer)));
        if (ret < 0) {
            fprintf(stderr, "发送数据失败\n");
            return ret;
        }

        videoSendThread = new std::thread(&Rtsp::sendVideo, this, transportStreamPacketNumber - 1);
        audioSendThread = new std::thread(&Rtsp::sendAudio, this, transportStreamPacketNumber - 1);

    } else if (strcmp(method, "TEARDOWN") == 0) {
        //结束会话请求，该请求会停止所有媒体流，并释放服务器上的相关会话数据。
        /*这里推流端和拉流端都会发送这个请求，要各自释放各自的资源*/
        sprintf(responseBuffer,
                "RTSP/1.0 200 OK\r\n"
                "CSeq: %s\r\n"
                "Date: %s\r\n"
                "Server: XiaoFeng\r\n"
                "Session: %s\r\n"
                "\r\n",
                obj["CSeq"].c_str(),
                generatorDate().c_str(),
                obj["Session"].c_str()
        );
        ret = TcpSocket::sendData(clientSocket, reinterpret_cast<uint8_t *>(responseBuffer),
                                  static_cast<int>(strlen(responseBuffer)));
        if (ret < 0) {
            fprintf(stderr, "发送数据失败\n");
            return ret;
        }
        /*用户发送请求，结束会话，并且释放服务器资源*/

        stopFlag = false;
    } else if (strcmp(method, "GET_PARAMETER") == 0) {
        printf("method = %s\n", method);
        printf("url = %s\n", url);
        /*保持连接用的*/
        /*在暂停流媒体播放，定期发送GET_PARAMETER作为心跳包维持连接*/
        sprintf(responseBuffer,
                "RTSP/1.0 200 OK\r\n"
                "CSeq: %s\r\n"
                "Date: %s\r\n"
                "Server: XiaoFeng\r\n"
                "Session: %s\r\n"
                "\r\n",
                obj["CSeq"].c_str(),
                generatorDate().c_str(),
                obj["Session"].c_str()
        );
        ret = TcpSocket::sendData(clientSocket, reinterpret_cast<uint8_t *>(responseBuffer),
                                  static_cast<int>(strlen(responseBuffer)));
        if (ret < 0) {
            fprintf(stderr, "发送数据失败 -> option\n");
            return ret;
        }
    } else {
        fprintf(stderr, "解析错误\n");
        fprintf(stderr, "不支持解析method = %s\n", method);
        return -1;
    }
    return 0;
}


int Rtsp::receiveData(std::string &packet, const std::string &session) const {


    int ret;

    Info &info = flowInfo[session];
    RtspReceiveData receive(info.transportStreamPacketNumber);
    ret = receive.init(clientSocket, info.dir, videoChannel, audioChannel);
    if (ret < 0) {
        fprintf(stderr, "receive.init失败\n");
        return ret;
    }
    ret = receive.writeVideoData(info.sdp.spsData, info.sdp.spsSize);
    if (ret < 0) {
        fprintf(stderr, "receive.writeData失败\n");
        return ret;
    }
    ret = receive.writeVideoData(info.sdp.ppsData, info.sdp.ppsSize);
    if (ret < 0) {
        fprintf(stderr, "receive.writeData失败\n");
        return ret;
    }
    ret = receive.writeAudioData(info.sdp.audioObjectType, info.sdp.samplingFrequencyIndex,
                                 info.sdp.channelConfiguration);
    if (ret < 0) {
        fprintf(stderr, "receive.writeAudioData失败\n");
        return ret;
    }
    ret = receive.receiveData(packet);
    if (ret < 0) {
        fprintf(stderr, "receive.receiveData失败\n");
        return ret;
    }

    /*TransportPacket ts;
    *//*接收rtsp数据，存为ts*//*
    ts.init("test/");

    *//*一个rtp包最大大小不会超过16个bit也就是65535*//*
    uint8_t *buffer = new uint8_t[65535]; // NOLINT(modernize-use-auto)
    // todo delete buffer
    *//*packet这里面有可能还有剩余的字节没读取完*//*
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
            *//*这里跳过这个循环，因为有可能，还取不到四个字节数据*//*
            continue;
        }
        uint8_t magic = buffer[0];
        if (magic != '$') {
            fprintf(stderr, "读取错误，没找到分隔符\n");
            return -1;
        }
        uint8_t channel = buffer[1];
        *//*这个rtp包剩余数据大小*//*
        uint16_t length = buffer[2] << 8 | buffer[3];

        rtpBuffer = buffer + 4;
        rtpBufferSize = bufferSize - 4;
        while (true) {
            if (rtpBufferSize < length) {
                *//*不够rtp包大小继续取数据，直到取到为止*//*
                ret = TcpSocket::receive(clientSocket, (char *) (rtpBuffer + rtpBufferSize), size);
                if (ret < 0) {
                    fprintf(stderr, "TcpSocket::receive 失败\n");
                    return ret;
                }
                rtpBufferSize += size;
            } else {
                *//*处理数据*//*
                ret = disposeRtpData(ts, rtpBuffer, rtpBufferSize, channel, length);
                if (ret < 0) {
                    fprintf(stderr, "处理rtp包失败\n");
                    return ret;
                }
                rtpBufferSize -= length;
                *//*这里不用使用memmove,不会发生内存重叠*//*
                memcpy(buffer, rtpBuffer + length, rtpBufferSize);
                bufferSize = rtpBufferSize;
                break;
            }
        }
    }*/


    return 0;
}

static constexpr uint8_t startCode[4] = {0, 0, 0, 1};

/*int Rtsp::disposeRtpData(TransportPacket &ts, uint8_t *rtpBuffer, uint32_t rtpBufferSize, uint8_t channel,
                         uint16_t length) {
    int ret;
    if (channel == videoChannel) {
        ReadStream rs(rtpBuffer, rtpBufferSize);
        *//*获取rtp header*//*
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
            *//**//*
            uint8_t start = rs.readMultiBit(1);
            uint8_t end = rs.readMultiBit(1);
            *//*reserved = 0*//*
            rs.readMultiBit(1);

            nal_unit_type = rs.readMultiBit(5);
            length -= 1;


            if (start == 1 && end == 0) {
                *(rs.currentPtr - 1) = forbidden_zero_bit << 7 | (nal_ref_idc << 5) | nal_unit_type;
                *//*这里手动给nalu加上起始码，固定0001*//*
                memcpy(nalUintData, startCode, 4);
                memcpy(nalUintData + 4, rs.currentPtr - 1, length + 1);
                nalUintOffset = 4 + length + 1;
            } else if (start == 0 && end == 0) {
                memcpy(nalUintData + nalUintOffset, rs.currentPtr, length);
                nalUintOffset += length;
            } else if (start == 0 && end == 1) {
                memcpy(nalUintData + nalUintOffset, rs.currentPtr, length);
                nalUintOffset += length;
                *//*这里还可以依靠mark标记来确定一帧的最后一个slice*//*
                ret = nalReader.test1(frame, nalUintData, nalUintOffset, 4);
                if (ret < 0) {
                    fprintf(stderr, "nalReader.getPicture 失败\n");
                    return -1;
                }
                if (frame->pictureFinishFlag) {
                    *//*获取到完整的一帧,做对应操作*//*
                    // ret = ts.writeTransportStream(frame, transportStreamPacketNumber);
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

            uint8_t *ptr = rs.currentPtr - 5;
            memcpy(ptr, startCode, 4);
            *(ptr + 4) = forbidden_zero_bit << 7 | (nal_ref_idc << 5) | nal_unit_type;
            ret = nalReader.test1(frame, ptr, length + 5, 4);
            if (ret < 0) {
                fprintf(stderr, "nalReader.getPicture 失败\n");
                return -1;
            }


            if (frame->pictureFinishFlag) {
//                ret = ts.writeTransportStream(frame, transportStreamPacketNumber);
                if (ret < 0) {
                    fprintf(stderr, "ts.writeVideo 失败\n");
                    return -1;
                }
            }

        }
    } else if (channel == audioChannel) {
        //  printf("audiochanggel\n");
        ReadStream rs(rtpBuffer, rtpBufferSize);
        *//*获取rtp header*//*
        ret = getRtpHeader(rs);
        if (ret < 0) {
            return ret;
        }

        uint16_t headerLength = rs.readMultiBit(16) / 16;

        std::vector<uint16_t> sizeList;
        for (int i = 0; i < headerLength; ++i) {
            uint16_t sizeLength = rs.readMultiBit(13);
            uint8_t indexDelta = rs.readMultiBit(3);
            sizeList.push_back(sizeLength);
        }
        for (uint16_t aacSize: sizeList) {
            AdtsHeader::setFrameLength(aacSize + 7);
            memcpy(rs.currentPtr - 7, AdtsHeader::header, 7);

            adtsReader.disposeAudio(adtsHeader, rs.currentPtr - 7, aacSize + 7);

            ret = ts.writeAudioFrame(adtsHeader);
            if (ret < 0) {
                fprintf(stderr, "写入音频失败\n");
                return -1;
            }
            rs.setBytePtr(aacSize);
        }


    } else {
        printf("rtcp channel = %d\n", channel);
    }

    return 0;
}*/


/*发送音频和视频的函数*/
int Rtsp::sendVideo(uint32_t number) {
    int ret;

    NALReader reader;
    ret = reader.init1(dir, number, 90000);
    if (ret < 0) {
        fprintf(stderr, "reader.init1失败\n");
        return ret;
    }


    RtpPacket videoPacket;
    videoPacket.init(NALReader::MAX_BUFFER_SIZE, RTP_PAYLOAD_TYPE_H264);

    NALPicture *picture = reader.allocPicture();
    if (picture == nullptr) {
        fprintf(stderr, "分配picture失败\n");
        return -1;
    }
    uint32_t idx = 0;
    std::chrono::duration<int, std::milli> elapsed{};
    std::chrono::time_point<std::chrono::high_resolution_clock> start;
    while (stopVideoSendFlag) {
        start = std::chrono::high_resolution_clock::now();
        ret = reader.getVideoFrame1(picture);
        if (ret < 0) {
            fprintf(stderr, "获取视频数据失败\n");
            stopFlag = false;
            return ret;
        }
        videoPacket.timestamp = picture->dts;
        for (int i = 0; i < picture->data.size(); ++i) {
            const Frame &nalUint = picture->data[i];
            ret = videoPacket.sendVideoFrame(clientSocket, nalUint.data, nalUint.nalUintSize,
                                             i == (picture->data.size() - 1), videoChannel);
            if (ret < 0) {
                fprintf(stderr, "发送视频数据失败\n");
                stopFlag = false;
                return ret;
            }
        }
        /*一次发送一秒数据过去*/
        while (idx++ < picture->sliceHeader.sps.fps && stopVideoSendFlag) {
            ret = reader.getVideoFrame1(picture);
            if (ret < 0) {
                fprintf(stderr, "获取视频数据失败\n");
                stopFlag = false;
                return ret;
            }
            videoPacket.timestamp = picture->dts;
            for (int i = 0; i < picture->data.size(); ++i) {
                const Frame &nalUint = picture->data[i];
                ret = videoPacket.sendVideoFrame(clientSocket, nalUint.data, nalUint.nalUintSize,
                                                 i == (picture->data.size() - 1), videoChannel);
                if (ret < 0) {
                    fprintf(stderr, "发送视频数据失败\n");
                    stopFlag = false;
                    return ret;
                }
            }
        }
        idx = 0;
        elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::high_resolution_clock::now() - start);
        //printf("运行了多久 %d\n", elapsed.count());
        std::this_thread::sleep_for(std::chrono::milliseconds(1000 - elapsed.count()));
    }







    /* todo  这里父线程终止，当前线程也应该退出，释放资源，等会儿做 */
//    std::chrono::duration<int, std::micro> elapsed{};
//    auto lastTime = std::chrono::high_resolution_clock::now();
//    /*这里如果父线程退出的话，这里自然会退出，但是如果是子线程出错，想要退出并且关闭这个tcp链接*/
//    while (stopVideoSendFlag) {
//        ret = reader.getVideoFrame1(picture);
//        if (ret < 0) {
//            fprintf(stderr, "获取视频数据失败\n");
//            stopFlag = false;
//            return ret;
//        }
//        videoPacket.timestamp = picture->dts;
//        for (int i = 0; i < picture->data.size(); ++i) {
//            const Frame &nalUint = picture->data[i];
//            ret = videoPacket.sendVideoFrame(clientSocket, nalUint.data, nalUint.nalUintSize,
//                                             i == (picture->data.size() - 1), videoChannel);
//            if (ret < 0) {
//                fprintf(stderr, "发送视频数据失败\n");
//                stopFlag = false;
//                return ret;
//            }
//        }
//
//        elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
//                std::chrono::high_resolution_clock::now() - lastTime);
//
//        //  std::this_thread::sleep_for(std::chrono::microseconds(picture->interval - elapsed.count() - 9000));
//        std::this_thread::sleep_for(std::chrono::microseconds(picture->interval - elapsed.count()));
//
//        lastTime = std::chrono::high_resolution_clock::now();
//    }
    /*  RtpPacket videoPacket;
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
      printf("视频发送完成\n");*/
    return 0;
}

int Rtsp::sendAudio(uint32_t number) {
    int ret;

    AdtsReader reader;
    ret = reader.init1(dir, number);
    if (ret < 0) {
        fprintf(stderr, "reader.init1失败\n");
        return ret;
    }

    RtpPacket audioPacket;
    audioPacket.init(AdtsReader::MAX_BUFFER_SIZE, RTP_PAYLOAD_TYPE_AAC);


    AdtsHeader header;
//    ret = reader.getAudioParameter(header);
//    if (ret < 0) {
//        fprintf(stderr, "获取音频参数失败\n");
//        return ret;
//    }


    uint32_t fps;
    uint32_t idx = 0;
    std::chrono::duration<int, std::milli> elapsed{};
    std::chrono::time_point<std::chrono::high_resolution_clock> start;
    while (stopAudioSendFlag) {
        start = std::chrono::high_resolution_clock::now();

        ret = reader.getAudioFrame1(header);
        if (ret < 0) {
            fprintf(stderr, "获取audio frame失败\n");
            return ret;
        }
        audioPacket.timestamp = header.dts;
        /*这里是把音频数据发出去*/
        ret = audioPacket.sendAudioPacket(clientSocket, header.data, header.size, audioChannel);
        if (ret < 0) {
            fprintf(stderr, "发送音频包失败\n");
            //  audioSendError = true;
            return ret;
        }
        /*得到一秒几帧*/
        fps = header.sample_rate / 1024;
        while (idx++ < fps && stopAudioSendFlag) {
            /*这里是获取一帧音频数据*/
            ret = reader.getAudioFrame1(header);
            if (ret < 0) {
                fprintf(stderr, "获取audio frame失败\n");
                return ret;
            }
            audioPacket.timestamp = header.dts;
            /*这里是把音频数据发出去*/
            ret = audioPacket.sendAudioPacket(clientSocket, header.data, header.size, audioChannel);
            if (ret < 0) {
                fprintf(stderr, "发送音频包失败\n");
                //  audioSendError = true;
                return ret;
            }

        }
        idx = 0;
        elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::high_resolution_clock::now() - start);
        //printf("运行了多久 %d\n", elapsed.count());
        std::this_thread::sleep_for(std::chrono::milliseconds(1000 - elapsed.count()));
    }



//    std::chrono::duration<int, std::micro> elapsed{};
//    auto lastTime = std::chrono::high_resolution_clock::now();
//
//    /*如果是44100的采样率，一秒钟就有43帧，那么我可以一次性发送43帧，然后间隔一秒再去发送43帧*/
//    while (stopAudioSendFlag) {
//        /*这里是获取一帧音频数据*/
//        ret = reader.getAudioFrame1(header);
//        if (ret < 0) {
//            fprintf(stderr, "获取audio frame失败\n");
//            return ret;
//        }
//        audioPacket.timestamp = header.dts;
//        /*这里是把音频数据发出去*/
//        ret = audioPacket.sendAudioPacket(clientSocket, header.data, header.size, audioChannel);
//        if (ret < 0) {
//            fprintf(stderr, "发送音频包失败\n");
//            //  audioSendError = true;
//            return ret;
//        }
//
//        /*当前时间减去上个时间*/
//        elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
//                std::chrono::high_resolution_clock::now() - lastTime);
//
//        // std::this_thread::sleep_for(std::chrono::microseconds(header.interval - elapsed.count() - 8100));
//        std::this_thread::sleep_for(std::chrono::microseconds(header.interval - elapsed.count()));
//        /*记录下上一帧发送完的时间*/
//        lastTime = std::chrono::high_resolution_clock::now();
//    }
    return 0;
    /* int ret;

     RtpPacket audioPacket;
     audioPacket.init(AdtsReader::MAX_BUFFER_SIZE, RTP_PAYLOAD_TYPE_AAC);

     AdtsReader reader;
     ret = reader.init("");
     if (ret < 0) {
         fprintf(stderr, "init audio failed\n");
         // audioSendError = true;
         return ret;
     }


     AdtsHeader header;
     bool flag = true;
     while (flag) {
         ret = reader.getAudioFrame(header, flag);
         if (ret < 0) {
             fprintf(stderr, "获取audio frame失败\n");
             //   audioSendError = true;
             return ret;
         }
         ret = audioPacket.sendAudioPacket(clientSocket, header.data, header.size, audioChannel);
         if (ret < 0) {
             fprintf(stderr, "发送音频包失败\n");
             //  audioSendError = true;
             return ret;
         }

         audioPacket.timestamp += 1024;
         if (audioPacket.timestamp >= 4294967295) {
             audioPacket.timestamp = 0;
         }
         std::this_thread::sleep_for(std::chrono::milliseconds(10));
     }
     printf("音频发送完成\n");
     return 0;*/
}

int Rtsp::parseSdp(const std::string &sdp, SdpInfo &sdpInfo) {
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
                ret = parseMediaLevel(i, list, sdpInfo);
                if (ret < 0) {
                    return ret;
                }
                break;
            }

        }
    }
    return 0;
}


int Rtsp::parseMediaLevel(int i, const std::vector<std::string> &list, SdpInfo &sdpInfo) {
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
                            std::map<std::string, std::string> obj = getObj(split(right, ";"), "=");
                            std::vector<std::string> sps_pps = split(obj["sprop-parameter-sets"], ",");

                            // uint8_t sps[50];
                            /* todo 等会儿删除 */
                            memcpy(sdpInfo.spsData, startCode, 4);
                            ret = base64_decode(sps_pps[0], sdpInfo.spsData + 4);
                            if (ret < 0) {
                                fprintf(stderr, "解析sps失败\n");
                                return ret;
                            }
                            sdpInfo.spsSize = ret + 4;
                            //                            spsSize = ret;
//                            ret = nalReader.test1(frame, sps, ret + 4, 4);
//                            if (ret < 0) {
//                                fprintf(stderr, "nalReader.getPicture 失败\n");
//                                return -1;
//                            }

                            // uint8_t pps[50];
                            memcpy(sdpInfo.ppsData, startCode, 4);
                            ret = base64_decode(sps_pps[1], sdpInfo.ppsData + 4);
                            if (ret < 0) {
                                fprintf(stderr, "解析pps失败\n");
                                return ret;
                            }
                            sdpInfo.ppsSize = ret + 4;
                            //                            ppsSize = ret;
//                            ret = nalReader.test1(frame, pps, ret + 4, 4);
//                            if (ret < 0) {
//                                fprintf(stderr, "nalReader.getPicture 失败\n");
//                                return -1;
//                            }
                        } else {
                            fprintf(stderr, "解析错误\n");
                            return -1;
                        }
                    } else if (sdpInfo.media[num]["type"] == "audio" && sdpInfo.media[num]["payloads"] == "97") {
                        if (left == "rtpmap:97") {
                            sdpInfo.media[num]["rtpmap"] = right;
                        } else if (left == "fmtp:97") {
                            //                            fmtp:97 profile-level-id=1;mode=AAC-hbr;sizelength=13;indexlength=3;indexdeltalength=3; config=119056E500
                            std::map<std::string, std::string> obj = getObj(split(right, ";"), "=");
                            sdpInfo.media[num]["fmtp"] = right;
                            ret = parseAACConfig(obj["config"], sdpInfo);
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


int Rtsp::parseAACConfig(const std::string &config, SdpInfo &sdpInfo) {
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
    sdpInfo.audioObjectType = std::stoi(bin.substr(0, 5), nullptr, 2); // 前5位，表示音频对象类型（Audio Object Type）
    sdpInfo.samplingFrequencyIndex = std::stoi(bin.substr(5, 4), nullptr, 2); // 后4位，表示采样率索引（Sampling Frequency Index）
    sdpInfo.channelConfiguration = std::stoi(bin.substr(9, 4), nullptr, 2); // 后4位，表示声道配置（Channel Configuration）
    uint8_t GASpecificConfig = std::stoi(bin.substr(13, 3), nullptr, 2); // 后3位，表示全局音频特定配置信息



    /*写入adts header*/
//    WriteStream ws(AdtsHeader::header, 7);
//    adtsHeader.setConfig(audioObjectType - 1, samplingFrequencyIndex, channelConfiguration, 0);
//    adtsHeader.writeAdtsHeader(ws);

    return 0;
}


Rtsp::~Rtsp() {
    printf("~Rtsp 析构\n");

    flowInfo.erase(uniqueSession);

    std::filesystem::remove_all(flowInfo[uniqueSession].dir);

    stopVideoSendFlag = false;
    stopAudioSendFlag = false;
    if (videoSendThread && videoSendThread->joinable()) {
        videoSendThread->join();
        delete videoSendThread;
    }

    if (audioSendThread && audioSendThread->joinable()) {
        audioSendThread->join();
        delete audioSendThread;
    }
}











