

#ifndef TS_TRANSPORTPACKET_H
#define TS_TRANSPORTPACKET_H


#include <cstdint>
#include <fstream>
#include <vector>
#include "SI.h"
#include "PES.h"
#include "bitStream/writeStream.h"
#include "bitStream/readStream.h"


class NALPicture;

class AdtsHeader;

struct TransportStreamInfo {
    std::string name;
    double duration;
};

class TransportPacket {

private:

    int transportStreamPacketNumber{0};
    std::string dir;
    uint32_t seq{0};
    double lastDuration{0};

    uint8_t *buffer{nullptr};
    WriteStream *ws{nullptr};

    std::ofstream m3u8FileSystem;
    std::ofstream transportStreamFileSystem;
    std::vector<TransportStreamInfo> list;


    PES pes;
    SI info;
    /*同步字段，固定是0x47*/
    uint8_t sync_byte{0x47};
    /*传输错误标记，在 TCP/UDP 场景，这个字段应该一直是 0,因为网络传输不管是UDP还是TCP都自带差错校验*/
    uint8_t transport_error_indicator{0};
    /*第一个ts包为1*/
    uint8_t payload_unit_start_indicator{0};
    /*传输优先级，0为低优先级，1为高优先级，通常取0*/
    uint8_t transport_priority{0};
    /*PID 可以判断 playload 里面是什么数据*/
    uint16_t PID{0};
    /*这个是限制字段，一般用做付费节目之类的，以前卫星电视，需要插一张卡，充钱才能看某些节目。互联网场景很少使用这个字段*/
    uint8_t transport_scrambling_control{0};
    /*可变字段标记，这个字段有 4 个值，10 跟 11 代表 后面有扩展的字段需要解析，01 跟 11 代表 这个 TS 包有 playload */
    /*解码器将丢弃adaptation_field_control字段设置为'00'的传输流数据包。如果是空包，adaptation_field_control的值应该设置为“01”。*/
    /*参见Table 2-5 – Adaptation field control values*/
    uint8_t adaptation_field_control{0};
    /*从0到15连续循环递增，代表连续的包，不一定从0开始*/
    uint8_t continuity_counter{0};

    uint8_t pointer_field{0};
/*adaptation_field*/
    uint8_t adaptation_field_length{1};
    uint8_t discontinuity_indicator{0};
    uint8_t random_access_indicator{0};
    uint8_t elementary_stream_priority_indicator{0};
    uint8_t PCR_flag{0};
    uint8_t OPCR_flag{0};
    uint8_t splicing_point_flag{0};
    uint8_t transport_private_data_flag{0};
    uint8_t adaptation_field_extension_flag{0};
    uint64_t program_clock_reference_base{0};
    uint16_t program_clock_reference_extension{0};
    uint64_t original_program_clock_reference_base{0};
    uint16_t original_program_clock_reference_extension{0};
    uint8_t splice_countdown{0};
    uint8_t transport_private_data_length{0};
    uint8_t *private_data_byte{nullptr};
    uint8_t adaptation_field_extension_length{0};
    uint8_t ltw_flag{0};
    uint8_t piecewise_rate_flag{0};
    uint8_t seamless_splice_flag{0};
    uint8_t ltw_valid_flag{0};
    uint16_t ltw_offset{0};
    uint32_t piecewise_rate{0};
    uint8_t splice_type{0};


    uint32_t videoPacketSize{0};
    uint32_t audioPacketSize{0};

    uint64_t time{0};
public:
    int init(std::string path);

    int writeTransportStream(const NALPicture *picture/*, int &transportStreamPacketNumber*/);

    int writeVideoFrame(const NALPicture *picture);

    int writeAudioFrame(const AdtsHeader &header);

    ~TransportPacket();

private:
    int writeTable();

    int writeServiceDescriptionTable();

    int writeProgramAssociationTable();

    int writeProgramMapTable();

    int transport_packet() const;

    int setTransportPacketConfig(uint8_t payloadUnitStartIndicator, uint16_t pid, uint8_t control, uint32_t counter);

    int setAdaptationFieldConfig(uint8_t randomAccessIndicator, uint16_t adaptationFieldLength, bool flag);

    int adaptation_field() const;

};


#endif //TS_TRANSPORTPACKET_H
