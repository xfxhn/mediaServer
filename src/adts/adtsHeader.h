
#ifndef MUX_ADTSHEADER_H
#define MUX_ADTSHEADER_H

#include <cstdint>

class ReadStream;

class WriteStream;

/*
 * profile
 * 该数据元素的解释取决于ID位的值。
 * 如果ID等于“1”，该字段包含与ISO/IEC 13818-7中定义的ADTS流中的profile字段相同的信息。
 * 如果ID等于“0”，这个元素表示MPEG-4音频对象类型(profile_ObjectType+1)，根据subsub1.5.2.1中定义的表。
 * */
class AdtsHeader {
public:


    static uint8_t header[7];
    bool finishFlag{false};
    uint8_t *data{nullptr};
    uint32_t size{0};
    uint64_t pts{0};
    uint64_t dts{0};
    int interval{0};
    double duration{0};
    /*fixed_header*/
    /*帧同步标识一个帧的开始，固定为0xFFF*/
    uint16_t syncword{0xFFF};
    /*MPEG标识符，如果ADTS流中的音频数据为MPEG-2 AAC，则设置为' 1 '
(参见ISO/IEC 13818-7)，如果音频数据是MPEG-4，则设置为' 0 '。另请参阅
ISO/IEC 11172-3，第2.4.2.3款。*/
    uint8_t ID{0};
    /*表示使用哪一层 11=Layer1 10=Layer2 01=Layer3 00=保留。参考mpeg-1 part3 2.4.2.3小节，这里是AAC，不兼容mpeg1音频，固定为00*/
    uint8_t layer{0};
    /* 是否在音频位流中添加了冗余以方便错误检测和隐藏(参见mpeg-1 part3 protection_bit)。标识是否进行误码校验。0表示有CRC校验，1表示没有CRC校验*/
    uint8_t protection_absent{1};
    /*标识使用哪个级别的AAC  id=0，这个值要减1*/
    uint8_t profile{0};
    /*标识使用的采样率的下标*/
    uint8_t sampling_frequency_index{0};
    /*对应的采样率*/
    uint32_t sample_rate{0};
    /*私有位，编码时设置为0，解码时忽略
      指示是否在音频位流中添加了冗余以方便
      错误检测和隐藏。 如果没有添加冗余，则等于'1'，如果添加了冗余，则等于'0'
     */
    uint8_t private_bit{0};

    /*标识声道数 通道配置
     * 如果如果channel_configuration大于0，则通道配置由表42中的' Default bitstream index number '给出，参见表8.5
     * 如果channel_configuration = 0,则通道配置未在标头中指定,
     * 并且必须由作为标头之后第一个 raw_data_block() 中的第一个句法元素的program_config_element() 或由隐式配置给出,
     * */
    uint8_t channel_configuration{0};
    /*编码时设置为0，解码时忽略*/
    uint8_t original_copy{0};
    /*编码时设置为0，解码时忽略*/
    uint8_t home{0};


    /*variable_header*/
    /*编码时设置为0，解码时忽略*/
    uint8_t copyright_identification_bit{0};
    /*编码时设置为0，解码时忽略*/
    uint8_t copyright_identification_start{0};
    /*ADTS帧长度包括ADTS长度和AAC声音数据长度的和。即 aac_frame_length = (protection_absent == 0 ? 9 : 7) + audio_data_length*/
    uint16_t frame_length{0};
    /*固定为0x7FF。表示是码率可变的码流*/
    uint16_t adts_buffer_fullness{0x7FF};
    /*表示当前帧有number_of_raw_data_blocks_in_frame + 1 个原始帧(一个AAC原始帧包含一段时间内1024个采样及相关数据)。*/
    uint8_t number_of_raw_data_blocks_in_frame{0};


public:
    /*ADTS 的固定标头。此标头中的信息不会因帧而异。它 在每一帧重复以允许随机访问比特流比特流*/
    int adts_fixed_header(ReadStream &rs);

    /*ADTS 的可变标头。该报头与固定报头一样每帧传输， 但包含随帧变化的数据*/
    int adts_variable_header(ReadStream &rs);

    void
    setConfig(uint8_t audioObjectType, uint8_t samplingFrequencyIndex, uint8_t channelConfiguration, uint16_t length);

    int writeAdtsHeader(WriteStream &ws) const;

    static int setFrameLength(int frameLength);
};


#endif //MUX_ADTSHEADER_H
