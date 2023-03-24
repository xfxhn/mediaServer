#include "demuxPacket.h"
#include "PES.h"
#include <cstdio>
#include <string>


int DemuxPacket::readFrame(ReadStream &rs) {
    int ret;
    sync_byte = rs.readMultiBit(8);
    transport_error_indicator = rs.readMultiBit(1);
    payload_unit_start_indicator = rs.readMultiBit(1);
    transport_priority = rs.readMultiBit(1);
    PID = rs.readMultiBit(13);
    transport_scrambling_control = rs.readMultiBit(2);
    adaptation_field_control = rs.readMultiBit(2);
    continuity_counter = rs.readMultiBit(4);

    if (adaptation_field_control == 2 || adaptation_field_control == 3) {
        ret = adaptation_field(rs);
        if (ret < 0) {
            fprintf(stderr, "解析adaptation_field失败\n");
            return ret;
        }
    }

    if (adaptation_field_control == 1 || adaptation_field_control == 3) {
        if (VIDEO_PID == PID || AUDIO_PID == PID) {
            if (payload_unit_start_indicator) {
                ret = PES::read_PES_packet(rs);
                if (ret < 0) {
                    fprintf(stderr, "解析PES_packet失败\n");
                    return ret;
                }
            }
            return PID;
        }
    }
    return 0;
}

int DemuxPacket::adaptation_field(ReadStream &rs) {
    /*指定紧接在adaptation_field_length后面的adaptation_field的字节数*/
    /*值0表示在传输流数据包中插入单个填充字节*/
    /*当adaptation_field_control值为'11'时，adaptation_field_length的取值范围为0 ~ 182。*/
    /*当adaptation_field_control值为'10'时，adaptation_field_length值为183*/
    /*对于携带PES报文的Transport Stream报文，当PES报文数据不足以完全填充Transport Stream报文有效负载字节时，需要进行填充*/
    adaptation_field_length = rs.readMultiBit(8);

    uint8_t N = adaptation_field_length;
    if (adaptation_field_length > 0) {
        discontinuity_indicator = rs.readBit();
        random_access_indicator = rs.readBit();
        elementary_stream_priority_indicator = rs.readBit();
        PCR_flag = rs.readBit();
        OPCR_flag = rs.readBit();
        splicing_point_flag = rs.readBit();
        transport_private_data_flag = rs.readBit();
        adaptation_field_extension_flag = rs.readBit();
        N -= 1;
        if (PCR_flag) {

            /*Mpeg-2规定的系统时钟频率为27MHz 也就是一秒27MHz*/
            /*
             * system_clock_frequency = 27000000
             * PCR_base(i) = ((system_clock_frequency*t(i)) / 300)%2^33 */
            /*
             * CR_base:以1/300 的系统时钟频率周期为单位，称之为program_clock_reference_base
             * PCR-base的作用:
             * a. 与PTS和DTS作比较, 当二者相同时, 相应的单元被显示或者解码.
             * b. 在解码器切换节目时,提供对解码器PCR计数器的初始值,以让该PCR值与PTS、DTS最大可能地达到相同的时间起点
             * */
            program_clock_reference_base = rs.readMultiBit(33);
            rs.readMultiBit(6);
            /*PCR_ext(i) = ((system_clock_frequency*t(i)) / 1)%300 */
            program_clock_reference_extension = rs.readMultiBit(9);

            /*PCR(i) = PCR_base(i)*300 + PCR_ext(i) */
            uint64_t PCR = program_clock_reference_base * 300 + program_clock_reference_extension;
            /*PCR 指示包含 program_clock_reference_base 最后一个 bit 的字节在系统目标解码器的输入端的预期到达时间*/
            N -= 6;
        }

        if (OPCR_flag) {
            original_program_clock_reference_base = rs.readMultiBit(33);
            rs.readMultiBit(6);
            original_program_clock_reference_extension = rs.readMultiBit(9);
            N -= 6;
        }
        if (splicing_point_flag) {
            splice_countdown = rs.readMultiBit(8);
            N -= 1;
        }
        if (transport_private_data_flag) {
            transport_private_data_length = rs.readMultiBit(8);
            private_data_byte = new uint8_t[transport_private_data_length];
            for (int i = 0; i < transport_private_data_length; i++) {
                private_data_byte[i] = rs.readMultiBit(8);
            }
            N -= 1 + transport_private_data_length;
        }


        rs.setBytePtr(N);
    }


    return 0;
}