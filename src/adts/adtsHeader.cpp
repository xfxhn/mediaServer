
#include "adtsHeader.h"

#include <cstdio>
#include "bitStream/readStream.h"
#include "bitStream/writeStream.h"

enum AudioObjectType {
    Main = 0,
    LC = 1,
    SSR = 2,
    LTP = 3,
    LD = 23
};
uint8_t AdtsHeader::header[7]{0};

constexpr static uint32_t adts_sample_rates[] = {
        96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 7350, 0, 0, 0
};

int AdtsHeader::adts_fixed_header(ReadStream &rs) {
    syncword = rs.readMultiBit(12);
    ID = rs.readBit();
    layer = rs.readMultiBit(2);
    protection_absent = rs.readMultiBit(1);
    if (protection_absent == 0) {
        fprintf(stderr, "不支持CRC校验\n");
        return -1;
    }
    profile = rs.readMultiBit(2);
    sampling_frequency_index = rs.readMultiBit(4);
    sample_rate = adts_sample_rates[sampling_frequency_index];
    private_bit = rs.readBit();
    channel_configuration = rs.readMultiBit(3);
    original_copy = rs.readBit();
    home = rs.readBit();

    return 0;
}

int AdtsHeader::adts_variable_header(ReadStream &rs) {
    copyright_identification_bit = rs.readBit();
    copyright_identification_start = rs.readBit();
    frame_length = rs.readMultiBit(13);
    adts_buffer_fullness = rs.readMultiBit(11);
    number_of_raw_data_blocks_in_frame = rs.readMultiBit(2);
    return 0;
}


void AdtsHeader::setConfig(uint8_t audioObjectType, uint8_t samplingFrequencyIndex, uint8_t channelConfiguration,
                           uint16_t length) {
    profile = audioObjectType;
    sampling_frequency_index = samplingFrequencyIndex;
    channel_configuration = channelConfiguration;
    frame_length = length;
    sample_rate = adts_sample_rates[sampling_frequency_index];
}

int AdtsHeader::writeAdtsHeader(WriteStream &ws) const {
    /*adts_fixed_header*/
    ws.writeMultiBit(12, syncword);
    ws.writeMultiBit(1, ID);
    ws.writeMultiBit(2, layer);
    ws.writeMultiBit(1, protection_absent);
    ws.writeMultiBit(2, profile);
    ws.writeMultiBit(4, sampling_frequency_index);
    ws.writeMultiBit(1, private_bit);
    ws.writeMultiBit(3, channel_configuration);
    ws.writeMultiBit(1, original_copy);
    ws.writeMultiBit(1, home);

    /*adts_variable_header*/
    ws.writeMultiBit(1, copyright_identification_bit);
    ws.writeMultiBit(1, copyright_identification_start);
    ws.writeMultiBit(13, frame_length);
    ws.writeMultiBit(11, adts_buffer_fullness);
    ws.writeMultiBit(2, number_of_raw_data_blocks_in_frame);
    return 0;
}

int AdtsHeader::setFrameLength(int frameLength) {
    // 检查参数是否有效
    if (frameLength <= 0) {
        return -1;
    }
    // 取出frameLength最前面的两位放在header[3]最后面两位
    header[3] = (header[3] & 0xFC) | ((frameLength & 0x1800) >> 11);
    // 取出frameLength中间的8位
    header[4] = ((frameLength & 0x7F8) >> 3);
    /*取出最后3位，放在header[5]前三位上*/
    header[5] = (header[5] & 0x1F) | ((frameLength & 0x7) << 5);
    return 0;
}


