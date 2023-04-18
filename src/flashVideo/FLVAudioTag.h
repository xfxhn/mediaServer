

#ifndef MEDIASERVER_FLVAUDIOTAG_H
#define MEDIASERVER_FLVAUDIOTAG_H

#include <cstdint>
#include "bitStream/writeStream.h"

class FLVAudioTag {
private:
    /*
  * SoundData格式。定义了以下值:
  * 0 = Linear PCM, platform endian
  * 1 = ADPCM
  * 2 = MP3
  * 3 = Linear PCM, little endian
  * 10 = AAC
  * */
    uint8_t SoundFormat{10};
    /*
        采样率 对于AAC来说，该值总是3
        0 = 5.5 kHz
        1 = 11 kHz
        2 = 22 kHz
        3 = 44 kHz
     */
    uint8_t SoundRate{3};
    /*
每个采样点数据的大小， 这个字段只左右于未压缩格式 ,PCM 就是未压缩格式。而AAC这类就是已经压缩过的格式了。这个字段总是1
0 = 8-bit samples
1 = 16-bit samples*/
    uint8_t SoundSize{1};
    /*
     单声道或立体声 对于AAC的话总是1
     0 = Mono sound
     1 = Stereo sound
     */
    uint8_t SoundType{1};
    /*
是AAC编码的才有这个值
0 = AAC sequence header
1 = AAC raw
     */
    uint8_t AACPacketType{0};
private:
    uint8_t audioObjectType;
    uint8_t channelConfiguration;
    uint8_t samplingFrequencyIndex;

    bool frameLengthFlag{false};
    bool dependsOnCoreCoder{false};
    bool extensionFlag{false};


    uint16_t syncExtensionType;
public:
    void setConfig(uint8_t packetType);

    void setConfigurationRecord(uint8_t profile, uint8_t index, uint8_t channel);

    int writeConfig(WriteStream &ws) const;

    int writeData(WriteStream &ws) const;
};


#endif //MEDIASERVER_FLVAUDIOTAG_H
