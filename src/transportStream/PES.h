
#ifndef TS_PES_H
#define TS_PES_H

#include <cstdint>
#include <fstream>

class WriteStream;

class ReadStream;

/*enum {
    program_stream_map = 0xBC,
    padding_stream = 0xBE,
    private_stream_2 = 0xBF,
    ECM_STREAM = 0xF0,
    EMM_STREAM = 0xF1,
    program_stream_directory = 0xFF,
    DSMCC_stream = 0xF2,
    E_STREAM = 0xF8
};*/

class PES {
private:
    uint32_t packet_start_code_prefix{0x000001};
    uint8_t stream_id{0xE0};
    uint16_t PES_packet_length{0};

    /*0是不加扰*/
    uint8_t PES_scrambling_control{0};
    /*优先级，多路*/
    uint8_t PES_priority{0};
    /**/
    uint8_t data_alignment_indicator{0};
    /*此为1 比特字段。置于‘1’时，它指示相关PES 包有效载荷的素材依靠版权所保护*/
    uint8_t copyright{0};
    /*置于‘1’时，相关PES 包有效载荷的内容是原始的。置于‘0’时，它指示相关PES 包有效载荷的内容是复制的。*/
    uint8_t original_or_copy{0};
    /*
     * 当PTS_DTS_flags 字段设置为‘10’时，PES 包头中PTS 字段存在。当PTS_DTS_flags 字段设置为‘11’时，PES 包头中PTS 字段和DTS 字段均存在。
     * 当PTS_DTS_flags字段设置为‘00’时，PES 包头中既无任何PTS 字段也无任何DTS 字段存在。值‘01’禁用
     * */
    uint8_t PTS_DTS_flags{0};
    uint8_t ESCR_flag{0};
    uint8_t ES_rate_flag{0};
    uint8_t DSM_trick_mode_flag{0};
    uint8_t additional_copy_info_flag{0};
    uint8_t PES_CRC_flag{0};
    uint8_t PES_extension_flag{0};
    uint8_t PES_header_data_length{0};


    uint64_t pts{0};
    uint64_t dts{0};
public:
    int PES_packet(WriteStream *ws) const;

    static int read_PES_packet(ReadStream &rs) ;

    int set_PTS_DTS_flags(uint8_t flags, uint64_t pts, uint64_t dts);
};


#endif //TS_PES_H
