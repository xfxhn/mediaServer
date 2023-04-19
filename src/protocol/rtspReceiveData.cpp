
#include "rtspReceiveData.h"
#include "log/logger.h"

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
static constexpr uint8_t startCode[4] = {0, 0, 0, 1};


int RtspReceiveData::init(SOCKET socket, const std::string &path, uint8_t video, uint8_t audio) {
    int ret;
    /*要写入的目录，和从第几个写入*/
    ret = packet.init(path, 0);
    if (ret < 0) {
        log_error("packet.init 失败");
        return ret;
    }


    videoChannel = video;
    audioChannel = audio;
    clientSocket = socket;
    /*一个rtp包最大大小不会超过16个bit也就是65535*/
    buffer = new uint8_t[65535];


    return 0;
}


int RtspReceiveData::receiveData(const std::string &msg) {
    int ret;
    if (buffer == nullptr) {
        fprintf(stderr, "请初始化\n");
        return -1;
    }

    memcpy(buffer, msg.c_str(), msg.length());
    uint32_t bufferSize = msg.length();

    uint8_t *rtpBuffer = nullptr;
    uint32_t rtpBufferSize = 0;

    int size = 0;

    while (true) {
        if (bufferSize < 4) {
            ret = TcpSocket::receive(clientSocket, (char *) buffer + bufferSize, size);
            if (ret < 0) {
                log_warn("TcpSocket::receive");
                return ret;
            }
            bufferSize += size;
            /*这里跳过这个循环，因为有可能，还取不到四个字节数据*/
            continue;
        }
        uint8_t magic = buffer[0];
        if (magic != '$') {
            log_warn("推流断开连接，没找到分割");
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
                    log_warn("TcpSocket::receive");
                    return ret;
                }
                rtpBufferSize += size;
            } else {
                /*处理数据*/
                ret = disposeRtpData(rtpBuffer, rtpBufferSize, channel, length);
                if (ret < 0) {
                    log_error("处理rtp包失败");
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


int RtspReceiveData::disposeRtpData(uint8_t *rtpBuffer, uint32_t rtpBufferSize, uint8_t channel, uint16_t length) {
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

                /*这里手动给nalu加上起始码，固定0001*/

                uint8_t *ptr = rs.currentPtr - 5;
                memcpy(ptr, startCode, 4);
                *(ptr + 4) = forbidden_zero_bit << 7 | (nal_ref_idc << 5) | nal_unit_type;
                ret = packet.writeFrame(ptr, length + 5, "video", false);
                if (ret < 0) {
                    log_error("packet.writeFrame失败");
                    return ret;
                }

            } else if (start == 0 && end == 0) {

                ret = packet.writeFrame(rs.currentPtr, length, "video", false);
                if (ret < 0) {
                    log_error("packet.writeFrame失败");
                    return ret;
                }
            } else if (start == 0 && end == 1) {
                ret = packet.writeFrame(rs.currentPtr, length, "video", true);
                if (ret < 0) {
                    log_error("packet.writeFrame失败");
                    return ret;
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

            ret = packet.writeFrame(ptr, length + 5, "video", true);
            if (ret < 0) {
                log_error("packet.writeFrame失败");
                return ret;
            }

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

        std::vector<uint16_t> sizeList;
        for (int i = 0; i < headerLength; ++i) {
            uint16_t sizeLength = rs.readMultiBit(13);
            uint8_t indexDelta = rs.readMultiBit(3);
            sizeList.push_back(sizeLength);
        }
        for (uint16_t size: sizeList) {
            ret = packet.writeFrame(rs.currentPtr - 7, size + 7, "audio", true);
            if (ret < 0) {
                log_error("packet.writeFrame失败");
                return ret;
            }
            rs.setBytePtr(size);
        }

    } else {
        printf("rtcp channel = %d\n", channel);
    }

    return 0;
}

int RtspReceiveData::getRtpHeader(ReadStream &rs) {
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

int RtspReceiveData::writeVideoData(uint8_t *data, uint8_t size) {
    return packet.setVideoParameter(data, size);

}

int
RtspReceiveData::writeAudioData(uint8_t audioObjectType, uint8_t samplingFrequencyIndex, uint8_t channelConfiguration) {


    return packet.setAudioParameter(audioObjectType, samplingFrequencyIndex, channelConfiguration);
}

RtspReceiveData::~RtspReceiveData() {
    delete[] buffer;
}
