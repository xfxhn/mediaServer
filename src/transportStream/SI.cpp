

#include "SI.h"

#include "bitStream/writeStream.h"

/*具体参考mpeg-1 part1 table 2-34*/
#define AVC 0x1B
#define AAC 0x0F

uint32_t SI::table[256] = {0};

SI::SI() {
    makeTable();
}

int SI::service_description_section(WriteStream *ws) {

    uint8_t *buffer = ws->currentPtr;
    ws->writeMultiBit(8, sdt.table_id);
    /*The section_syntax_indicator is a 1-bit field which shall be set to '1'.*/
    ws->writeMultiBit(1, sdt.section_syntax_indicator);
    /*所有“reserved_future_use”位均应设置为“1”。*/
    ws->writeMultiBit(1, 0x01);
    ws->writeMultiBit(2, 0x03);

    ws->writeMultiBit(12, sdt.section_length);
    ws->writeMultiBit(16, sdt.transport_stream_id);
    ws->writeMultiBit(2, 0x03);
    ws->writeMultiBit(5, sdt.version_number);
    ws->writeMultiBit(1, sdt.current_next_indicator);
    ws->writeMultiBit(8, sdt.section_number);
    ws->writeMultiBit(8, sdt.last_section_number);

    /*这个跟网络信息表相关*/
    ws->writeMultiBit(16, sdt.original_network_id);
    ws->writeMultiBit(8, 0xFF);

    //  for (int i = 0; i < rs.size - rs.position; ++i) {
    /*
     * 唯一标识TS中的一个业务，它与program_map_section中的program_number（参看PMT表结构）相同。
     * 但当业务类型为0x04时（即NVOD参考业务，service_id没有对应的program_number）。
*/
    ws->writeMultiBit(16, sdt.service_id);
    ws->writeMultiBit(6, 0x3F);
    ws->writeMultiBit(1, sdt.EIT_schedule_flag);
    ws->writeMultiBit(1, sdt.EIT_present_following_flag);
    /*指示服务状态 4表示运行状态*/
    ws->writeMultiBit(3, sdt.running_status);
    ws->writeMultiBit(1, sdt.free_CA_mode);

    ws->writeMultiBit(12, sdt.descriptors_loop_length);
    service_descriptor(ws);
    //  }
    uint8_t size = ws->currentPtr - buffer;

    uint32_t CRC_32 = crc32Calculate(buffer, size);

    ws->writeMultiBit(32, CRC_32);
    return 0;
}

int SI::service_descriptor(WriteStream *ws) const {
    /*0x48表示service_descriptor 参见Table 12: Possible locations of descriptors*/
    ws->writeMultiBit(8, sdt.descriptor_tag);
    ws->writeMultiBit(8, sdt.descriptor_length);
    /*参见Table 50: Service type coding，0x01表示数字电视服务*/
    ws->writeMultiBit(8, sdt.service_type);


    /*服务提供者名称*/
    ws->writeMultiBit(8, sdt.service_provider_name_length);
    ws->setString("xiaofeng", 8);

    /*服务名称*/
    ws->writeMultiBit(8, sdt.service_name_length);
    ws->setString("Service01", 9);
    return 0;
}

int SI::program_association_section(WriteStream *ws) const {
    uint8_t *buffer = ws->currentPtr;

    ws->writeMultiBit(8, pat.table_id);
    ws->writeMultiBit(1, pat.section_syntax_indicator);
    ws->writeMultiBit(1, 0);
    ws->writeMultiBit(2, 3);
    /*
     * 这是一个12位的字段，前两位是'00'。
     * 其余10位指定section的字节数，从section_length字段后面开始，包括CRC。该字段的值不能超过1021 (0x3FD)。
     * */
    ws->writeMultiBit(12, pat.section_length);
    /*该传输流的ID，区别于一个网络中其它多路复用的流。*/
    ws->writeMultiBit(16, pat.transport_stream_id);
    ws->writeMultiBit(2, 3);
    /*5bits版本号码，标注当前节目的版本．这是个非常有用的参数，当检测到这个字段改变时，说明ＴＳ流中的节目已经变化 了，程序必须重新搜索节目*/
    ws->writeMultiBit(5, pat.version_number);
    /*表是当前这个表有效，还是下一个表有效*/
    ws->writeMultiBit(1, pat.current_next_indicator);
    /*分段的号码。PAT可能分为多段传输，第一段为00，以后每个分段加1，最多可能有256个分段*/
    ws->writeMultiBit(8, pat.section_number);
    /*
     * 最后段号码(section_number和 last_section_number的功能是当PAT内容>184字节时，
     * PAT表会分成多个段(sections),解复用程序必须在全部接 收完成后再进行PAT的分析)
     * */
    ws->writeMultiBit(8, pat.last_section_number);
/*
         * Program_number是一个16位字段。它指定program_map_PID适用于的程序。
         * 当设置为0x0000时，下面的PID引用将是网络PID。
         * 对于所有其他情况，此字段的值是用户定义的。
         * 在程序关联表的一个版本中，该字段不得使用任何单一值超过一次。
         * */
    ws->writeMultiBit(16, pat.program_number);
    ws->writeMultiBit(3, 7);
    /*定义program_map_PID 但是范围必须在0x0010 - 0x1FFE 以内，具体参考Table 2-3 –  PID table*/
    ws->writeMultiBit(13, pat.program_map_PID);
    uint8_t size = ws->currentPtr - buffer;

    uint32_t CRC_32 = crc32Calculate(buffer, size);


    /*本段的CRC校验值，一般是会忽略的*/
    ws->writeMultiBit(32, CRC_32);//pat.CRC_32
    return 0;
}

/*如果 TS 流中包含多个节目，那么就会有多个 PMT 表*/
int SI::program_map_section(WriteStream *ws) const {
    uint8_t *buffer = ws->currentPtr;


    ws->writeMultiBit(8, pmt.table_id);
    ws->writeMultiBit(1, pmt.section_syntax_indicator);
    ws->writeMultiBit(1, 0);
    ws->writeMultiBit(2, 3);
    ws->writeMultiBit(12, pmt.section_length);


    /*频道号码,表示当前的PMT关联到的频道.换句话就是说,当前描述的是program_number频道的信息*/
    ws->writeMultiBit(16, pat.program_number);
    ws->writeMultiBit(2, 3);
    /*版本号码,如果PMT内容有更新,则version_number会递增1通知解复用程序需要重新接收节目信息,否则 version_number是固定不变的*/
    ws->writeMultiBit(5, pmt.version_number);

    ws->writeMultiBit(1, pmt.current_next_indicator);
    ws->writeMultiBit(8, pmt.section_number);
    ws->writeMultiBit(8, pmt.last_section_number);
    ws->writeMultiBit(3, 7);
    /*PCR（节目时钟参考）所在TS分组的PID，根据PID可以去搜索相应的TS分组，解出PCR信息*/
    ws->writeMultiBit(13, pmt.PCR_PID);//256
    ws->writeMultiBit(4, 15);
    /*program_info_length*/
    ws->writeMultiBit(12, 0);


    ws->writeMultiBit(8, AVC);
    ws->writeMultiBit(3, 7);
    ws->writeMultiBit(13, pmt.videoPid);
    ws->writeMultiBit(4, 15);
    /*ES_info_length*/
    ws->writeMultiBit(12, 0);

    ws->writeMultiBit(8, AAC);
    ws->writeMultiBit(3, 7);
    ws->writeMultiBit(13, pmt.audioPid);
    ws->writeMultiBit(4, 15);
    /*ES_info_length*/
    ws->writeMultiBit(12, 0);

    uint8_t size = ws->currentPtr - buffer;

    uint32_t CRC_32 = crc32Calculate(buffer, size);
    ws->writeMultiBit(32, CRC_32);//pmt.CRC_32

    return 0;
}


void SI::makeTable() {
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t k = 0;
        for (uint32_t j = (i << 24) | 0x800000; j != 0x80000000; j <<= 1) {
            k = (k << 1) ^ (((k ^ j) & 0x80000000) ? 0x04c11db7 : 0);
        }
        table[i] = k;
    }
}


uint32_t SI::crc32Calculate(uint8_t *buffer, uint32_t size) {
    uint32_t crc32_reg = 0xFFFFFFFF;
    for (uint32_t i = 0; i < size; i++) {
        crc32_reg = (crc32_reg << 8) ^ table[((crc32_reg >> 24) ^ *buffer++) & 0xFF];
    }
    return crc32_reg;
}






