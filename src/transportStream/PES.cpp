
#include "PES.h"
#include "writeStream.h"
#include "readStream.h"


int PES::PES_packet(WriteStream *ws) const {
    ws->writeMultiBit(24, packet_start_code_prefix);

    ws->writeMultiBit(8, stream_id);
    /*
     * PES_packet_length顾名思义就是PES包的长度，但是注意，它是2个字节存储的，这意味着，最大只能表示65535，
     * 一旦视频帧很大，超过这个长度，怎么办，就把PES_packet_length置为0，这是ISO标准规定的。
     * 所以在解析的时候，不能以PES_packet_length为准，要参考payload_unit_start_indicator。
*/
    ws->writeMultiBit(16, PES_packet_length);


    ws->writeMultiBit(2, 2); //'10'

    ws->writeMultiBit(2, PES_scrambling_control);
    ws->writeMultiBit(1, PES_priority);
    ws->writeMultiBit(1, data_alignment_indicator);
    ws->writeMultiBit(1, copyright);
    ws->writeMultiBit(1, original_or_copy);
    ws->writeMultiBit(2, PTS_DTS_flags);
    ws->writeMultiBit(1, ESCR_flag);
    ws->writeMultiBit(1, ES_rate_flag);
    ws->writeMultiBit(1, DSM_trick_mode_flag);
    ws->writeMultiBit(1, additional_copy_info_flag);
    ws->writeMultiBit(1, PES_CRC_flag);
    ws->writeMultiBit(1, PES_extension_flag);
    ws->writeMultiBit(8, PES_header_data_length);

    if (PTS_DTS_flags == 2) {
        ws->writeMultiBit(4, 2);//'0010'
        uint8_t pts32_30 = pts >> 30;
        ws->writeMultiBit(3, pts32_30);
        ws->writeMultiBit(1, 1); //mark bit
        uint16_t pts29_15 = pts >> 15 & 0x7FFF;
        ws->writeMultiBit(15, pts29_15);
        ws->writeMultiBit(1, 1); //mark bit
        uint16_t pts14_0 = pts & 0x7FFF;
        ws->writeMultiBit(15, pts14_0);
        ws->writeMultiBit(1, 1); //mark bit
    }

    if (PTS_DTS_flags == 3) {
        ws->writeMultiBit(4, 3);//'0011'
        uint8_t pts32_30 = pts >> 30;
        ws->writeMultiBit(3, pts32_30);
        ws->writeMultiBit(1, 1); //mark bit
        uint16_t pts29_15 = pts >> 15 & 0x7FFF;
        ws->writeMultiBit(15, pts29_15);
        ws->writeMultiBit(1, 1); //mark bit
        uint16_t pts14_0 = pts & 0x7FFF;
        ws->writeMultiBit(15, pts14_0);
        ws->writeMultiBit(1, 1); //mark bit


        ws->writeMultiBit(4, 1);//'0001'
        uint8_t dts32_30 = dts >> 30;
        ws->writeMultiBit(3, dts32_30);
        ws->writeMultiBit(1, 1); //mark bit
        uint16_t dts29_15 = dts >> 15 & 0x7FFF;
        ws->writeMultiBit(15, dts29_15);
        ws->writeMultiBit(1, 1); //mark bit
        uint16_t dts14_0 = dts & 0x7FFF;
        ws->writeMultiBit(15, dts14_0);
        ws->writeMultiBit(1, 1); //mark bit

    }


    return 0;
}

int PES::read_PES_packet(ReadStream &rs) {
    /*packet_start_code_prefix = rs.readMultiBit(24);*/
    rs.setBytePtr(3);

    /*stream_id = rs.readMultiBit(8);*/
    rs.setBytePtr(1);

    /*PES_packet_length = rs.readMultiBit(16);*/
    rs.setBytePtr(2);

    rs.readMultiBit(2);
    /*PES_scrambling_control =*/ rs.readMultiBit(2);
    /*PES_priority =*/ rs.readMultiBit(1);
    /*data_alignment_indicator =*/ rs.readMultiBit(1);
    /*copyright =*/ rs.readMultiBit(1);
    /*original_or_copy =*/ rs.readMultiBit(1);
    uint8_t flags = rs.readMultiBit(2);
    /*ESCR_flag =*/ rs.readMultiBit(1);
    /*ES_rate_flag =*/ rs.readMultiBit(1);
    /*DSM_trick_mode_flag =*/ rs.readMultiBit(1);
    /*additional_copy_info_flag =*/ rs.readMultiBit(1);
    /*PES_CRC_flag =*/ rs.readMultiBit(1);
    /*PES_extension_flag =*/ rs.readMultiBit(1);
    uint8_t length = rs.readMultiBit(8);

    uint8_t N = length;
    if (flags == 2) {
        rs.readMultiBit(4);
        uint8_t pts32_30 = rs.readMultiBit(3);
        rs.readBit();
        uint16_t pts29_15 = rs.readMultiBit(15);
        rs.readBit();
        uint16_t pts14_0 = rs.readMultiBit(15);
        rs.readBit();
        // pts = (pts32_30 << 30) | (pts29_15 << 15) | pts14_0;
        /*fprintf(stdout, "pts = %d\n", pts);*/
        N -= 5;
    }

    if (flags == 3) {
        rs.readMultiBit(4);
        uint8_t pts32_30 = rs.readMultiBit(3);
        rs.readBit();
        uint16_t pts29_15 = rs.readMultiBit(15);
        rs.readBit();
        uint16_t pts14_0 = rs.readMultiBit(15);
        rs.readBit();
        // pts = (pts32_30 << 30) | (pts29_15 << 15) | pts14_0;
        //fprintf(stdout, "pts = %d\n", pts);
        rs.readMultiBit(4);
        uint8_t dts32_30 = rs.readMultiBit(3);
        rs.readBit();
        uint16_t dts29_15 = rs.readMultiBit(15);
        rs.readBit();
        uint16_t dts14_0 = rs.readMultiBit(15);
        rs.readBit();
        // dts = (dts32_30 << 30) | (dts29_15 << 15) | dts14_0;
        N -= 10;
    }
    rs.setBytePtr(N);

/*    } else if (stream_id == program_stream_map
               || stream_id == private_stream_2
               || stream_id == ECM_STREAM
               || stream_id == EMM_STREAM
               || stream_id == program_stream_directory
               || stream_id == DSMCC_stream
               || stream_id == E_STREAM) {
        fprintf(stderr, "stream_id = %d\n", stream_id);
        return -1;
    } else if (stream_id == padding_stream) {
        *//*剩下的填充字节*//*
        fprintf(stderr, "没有PES头，剩下的都是填充\n");
        return 0;
    }*/
    return 0;
}

int PES::set_PTS_DTS_flags(uint8_t flags, uint64_t _pts, uint64_t _dts) {
    if (flags == 2) {
        PES_header_data_length = 5;
    } else if (flags == 3) {
        PES_header_data_length = 10;
    } else {
        fprintf(stderr, "设置PTS_DTS_flags错误\n");
        return -1;
    }
    pts = (_pts + 1500) * 90;
    dts = (_dts + 1400) * 90;
    PTS_DTS_flags = flags;
    return 0;
}

