
#ifndef RTSP_RTSP_H
#define RTSP_RTSP_H

#include <cstdint>
#include <map>
#include <vector>
#include <string>
#include <cstring>
#include <thread>

#include "TcpSocket.h"
#include "rtpPacket.h"
#include "adtsReader.h"
#include "NALReader.h"
#include "adtsHeader.h"
#include "NALPicture.h"
#include "transportPacket.h"

struct SdpInfo {
    std::string version;
    std::map<std::string, std::string> origin;
    std::string name;
    std::map<std::string, std::string> timing;
    std::map<std::string, std::string> media[2];
};


struct Info {
    std::string session;
    std::string dir;
    SdpInfo sdp;
    /*这个流写入到第几个ts包*/
    uint32_t transportStreamPacketNumber{0};
};

class ReadStream;

class Rtsp {
private:
//    std::ofstream fs;
//    NALDecodedPictureBuffer gop;

    AdtsHeader adtsHeader;
    NALPicture *frame{nullptr};
    NALReader nalReader;
    AdtsReader adtsReader;


private:

    /* byte 0 */
    /*数据源的个数（即源的个数），如果只有一个源那么此时的值为0*/
    uint8_t csrcLen{0};
    /*扩充位，如果设置为允许的话，固定头结构后面（即包的12个字节后面，有效负载的前面）紧跟着一个扩展头结构，该结构是已定义的一种格式*/
    uint8_t extension{0};

    /*如果P=1，则在该报文的尾部填充一个或多个额外的八位组，它们不是有效载荷的一部分  */
    uint8_t padding{0};
    /*版本号*/
    uint8_t version{2};

    /* byte 1 */
    /*用于说明RTP报文中有效载荷的类型，如GSM音频、JPEM图像等,在流媒体中大部分是用来区分音频流和视频流的，这样便于客户端进行解析*/
    uint8_t payloadType{0};
    /*不同的有效载荷有不同的含义，对于视频，标记一帧的结束；对于音频，标记会话的开始*/
    uint8_t marker{0};

    /* bytes 2,3 */
    /*用于标识发送者所发送的RTP报文的序列号，每发送一个报文，序列号增1。
     * 这个字段当下层的承载协议用UDP的时候，网络状况不好的时候可以用来检查丢包。
     * 同时出现网络抖动的情况可以用来对数据进行重新排序，序列号的初始值是随机的，同时音频包和视频包的sequence是分别记数的*/
    uint16_t seq{0};

    /* bytes 4-7 */
    /*必须使用90 kHz 时钟频率。时戳反映了该RTP报文的第一个八位组的采样时刻。接收者使用时戳来计算延迟和延迟抖动，并进行同步控制*/
    uint32_t timestamp{0};

    /* bytes 8-11 */
    /*用于标识同步信源。该标识符是随机选择的，参加同一视频会议的两个同步信源不能有相同的SSRC*/
    uint32_t ssrc{0};


    SOCKET clientSocket;

    std::thread *videoSendThread{nullptr};
    std::thread *audioSendThread{nullptr};
    std::thread *receiveThread{nullptr};

    uint8_t videoChannel{0};
    uint8_t audioChannel{0};

//    uint8_t audioObjectType;
//    uint8_t samplingFrequencyIndex;
//    uint8_t channelConfiguration;
//    uint8_t GASpecificConfig;




    uint8_t *nalUintData{nullptr};
    uint32_t nalUintOffset{0};

//    uint8_t *aacFrameData{nullptr};
//    uint8_t *aacHeader{nullptr};
//    uint8_t *aacData{nullptr};

    //std::vector<uint8_t> pps;
public:
    bool stopFlag{true};

    /* bool videoSendError{false};
     bool audioSendError{false};*/

    std::string aacConfig;
    std::string sprop_parameter_sets;
    std::string profileLevelId;

public:
    int init(SOCKET socket);

    int parseRtsp(std::string &packet, const std::string &data);

    ~Rtsp();

private:
    /*sps数据，带start code*/
    uint8_t sps[50];
    /*pps数据，带start code*/
    uint8_t pps[50];


    bool stopVideoSendFlag{true};
    bool stopAudioSendFlag{true};

    std::string dir{"test/"};

    /*
     double lastDuration{0};
     uint32_t packetNumber{0};
     std::ofstream transportStreamFileSystem;
     std::vector<TransportStreamInfo> list;

     int packageTransportStream(const NALPicture *picture);*/


    int receiveData(std::string &packet);

    int getRtpHeader(ReadStream &rs);

    int sendVideo(uint32_t number);


    int sendAudio(uint32_t number);

    int parseSdp(const std::string &sdp, SdpInfo sdpInfo);

    int parseMediaLevel(int i, const std::vector<std::string> &list, SdpInfo sdpInfo);

    int
    disposeRtpData(TransportPacket &ts, uint8_t *rtpBuffer, uint32_t rtpBufferSize, uint8_t channel, uint16_t length);

    /*int muxTransportStream(uint8_t channel, uint8_t *data, uint32_t size);*/

    int parseAACConfig(const std::string &config);


};


#endif //RTSP_RTSP_H
