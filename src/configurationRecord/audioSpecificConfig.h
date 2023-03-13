
#ifndef FLV_AUDIOSPECIFICCONFIG_H
#define FLV_AUDIOSPECIFICCONFIG_H

#include <cstdint>
#include <fstream>


class WriteStream;


class AudioSpecificConfig {


public:
    uint8_t audioObjectType{2};
    uint32_t sampleRate{44100};
    uint8_t channelConfiguration{2};

    uint8_t samplingFrequencyIndex{4};
    uint32_t samplingFrequency{};


    int sbrPresentFlag{-1};
    int psPresentFlag{-1};
    /*一个5bit字段，表示扩展音频对象类型。该对象类型对应于一个扩展工具，该工具用于增强底层的audioObjectType*/
    uint8_t extensionAudioObjectType{};
    uint8_t extensionSamplingFrequencyIndex{};
    uint32_t extensionSamplingFrequency{};
    uint8_t extensionChannelConfiguration{};


    bool frameLengthFlag{false};
    bool dependsOnCoreCoder{false};
    bool extensionFlag{false};


    uint16_t syncExtensionType{};
public:
    int setConfig(uint8_t type, uint8_t index, uint8_t channel);

    int write(WriteStream &ws) const;


};


#endif //FLV_AUDIOSPECIFICCONFIG_H
