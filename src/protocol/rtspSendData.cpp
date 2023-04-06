

#include "rtspSendData.h"
#include <fstream>
#include <cstdio>
#include <cstring>
#include "writeStream.h"


#define RTP_MAX_PKT_SIZE        1400

#define RTP_HEADER_SIZE         12

/*const char *_ip, uint16_t _port,*/
int RtpPacket::init(uint32_t maxFrameSize, uint8_t type) {
    /*ip = _ip;
    port = _port;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);*/

    /*rtp头加上一帧的最大数据加上载荷数据*/
    /*4是最大的AU头大小*/
    maxFrameRtpPacketSize = RTP_HEADER_SIZE + maxFrameSize + 4 + 2;
    /*提前分配内存，避免重新分配*/
    frameBuffer = new uint8_t[maxFrameRtpPacketSize];
    payloadType = type;

    return 0;
}


int RtpPacket::writeHeader(WriteStream &ws) const {
    ws.writeMultiBit(2, version);
    ws.writeMultiBit(1, padding);
    ws.writeMultiBit(1, extension);
    ws.writeMultiBit(4, csrcLen);

    ws.writeMultiBit(1, marker);
    ws.writeMultiBit(7, payloadType);

    ws.writeMultiBit(16, seq);
    ws.writeMultiBit(32, timestamp);
    ws.writeMultiBit(32, ssrc);
    return 0;
}

int RtpPacket::writePayload(WriteStream &ws, uint8_t *data, uint32_t dataSize) {

    memcpy(ws.currentPtr, data, dataSize);
    ws.setBytePtr(dataSize);

    return 0;
}

int RtpPacket::sendVideoFrame(SOCKET clientSocket, uint8_t *data, uint32_t size, uint8_t flag, uint8_t channel) {
    WriteStream ws(frameBuffer, size + RTP_HEADER_SIZE + 4 + 2);

    int ret;


    /*Udp协议的MTU为1500，超过了会导致Udp分片传输，而分片的缺点是丢了一个片，整包数据就废弃了*/
    if (size <= RTP_MAX_PKT_SIZE) {

        marker = flag;
        ws.writeMultiBit(8, '$');
        ws.writeMultiBit(8, channel);
        ws.writeMultiBit(16, size + RTP_HEADER_SIZE);
        writeHeader(ws);
        writePayload(ws, data, size);
        ret = TcpSocket::sendData(clientSocket, frameBuffer, (int) ws.position);
        if (ret < 0) {
            fprintf(stderr, "发送失败\n");
            return -1;
        }
        ++seq;
    } else {
        /*打 fu-a 包时，如果待拆分的 nalu 是 nalu stream 的最后一个 nalu，且 fu-a header end 位置 1 时，才能标识 marker*/
        /*先设置为0，不管这个nalu是不是一帧的最后一个*/
        marker = 0;
        /*分片封包方式*/
        /*28 FU-A 分片的单元*/
        const uint8_t nalUintHeader = data[0];
        /*这个nalu分成了几包 RTP_MAX_PKT_SIZE大小*/
        uint32_t pktNum = (size - 1) / RTP_MAX_PKT_SIZE;
        // 剩余不完整包的大小
        uint32_t remainPktSize = (size - 1) % RTP_MAX_PKT_SIZE;

        /*
         * 如果使用UDP协议，当IP层组包发生错误，那么包就会被丢弃，接收方无法重组数据报，将导致丢弃整个IP数据报
         * 相当于如果丢了一个包，或者错误，那么整个upd的数据报都会丢失，所以要控制数据大小，尽量不要分包
         * */

        /*跳过nalUintHeader 1字节*/
        uint32_t pos = 1;

        for (int i = 0; i < pktNum; ++i) {

            uint8_t FU_indicator = nalUintHeader & 0xE0 | 28;
            uint8_t FU_header = nalUintHeader & 0x1F;

            if (i == 0) {
                /*第一个包*/
                FU_header = FU_header | 0x80;
            } else if (remainPktSize == 0 && i == pktNum - 1) {
                /*最后一包，因为有可能刚好整除,整除remainPktSize=0*/
                FU_header = FU_header | 0x40;
                marker = flag;
            }

            ws.reset();
            ws.writeMultiBit(8, '$');
            ws.writeMultiBit(8, 0);
            ws.writeMultiBit(16, RTP_MAX_PKT_SIZE + 2 + RTP_HEADER_SIZE);

            writeHeader(ws);
            ws.writeMultiBit(8, FU_indicator);
            ws.writeMultiBit(8, FU_header);

            writePayload(ws, data + pos, RTP_MAX_PKT_SIZE);


            ret = TcpSocket::sendData(clientSocket, frameBuffer, (int) ws.position);
            if (ret < 0) {
                fprintf(stderr, "发送失败\n");
                return -1;
            }
            ++seq;
            pos += RTP_MAX_PKT_SIZE;
        }


        /*发送剩余数据，不足RTP_MAX_PKT_SIZE大的*/
        if (remainPktSize > 0) {
            ws.reset();
            marker = flag;
            ws.writeMultiBit(8, '$');
            ws.writeMultiBit(8, 0);
            ws.writeMultiBit(16, remainPktSize + 2 + RTP_HEADER_SIZE);

            writeHeader(ws);

            uint8_t FU_indicator = nalUintHeader & 0xE0 | 28;

            uint8_t FU_header = nalUintHeader & 0x1F | 0x40;

            ws.writeMultiBit(8, FU_indicator);
            ws.writeMultiBit(8, FU_header);

            writePayload(ws, data + pos, remainPktSize);

            ret = TcpSocket::sendData(clientSocket, frameBuffer, (int) ws.position);
            if (ret < 0) {
                fprintf(stderr, "发送失败\n");
                return -1;
            }
            ++seq;
        }

    }


    return 0;
}

int RtpPacket::sendAudioPacket(SOCKET clientSocket, uint8_t *data, uint32_t size, uint8_t channel) {

    int ret;

    marker = 1;/**/
    WriteStream ws(frameBuffer, size + RTP_HEADER_SIZE + 4 + 4);
    ws.writeMultiBit(8, '$');
    ws.writeMultiBit(8, channel);
    ws.writeMultiBit(16, size + RTP_HEADER_SIZE + 4);
    /*这里写入的rtp头，就是12字节那个*/
    writeHeader(ws);
    /*4个字节AU头*/
    /*
     * 头两个字节表示au-header的长度，单位是bit
     * 一个AU-header长度是两个字节（16bit）因为可以有多个au-header,
     * 所以AU-headers-length的值是 16的倍数，
     * 一般音频都是单个音频数据流的发送，所以AU-headers-length的值是16
     * */
    ws.writeMultiBit(16, 16);

    /*13个bit存放数据大小*/
    ws.writeMultiBit(13, size);
    ws.writeMultiBit(3, 0);


    writePayload(ws, data, size);


    ret = TcpSocket::sendData(clientSocket, frameBuffer, (int) ws.position);
    if (ret < 0) {
        fprintf(stderr, "发送失败\n");
        return ret;
    }
    ++seq;
    return 0;
}

RtpPacket::~RtpPacket() {

    delete[] frameBuffer;

}
