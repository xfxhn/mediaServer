

#include "FLVAudioTag.h"

void FLVAudioTag::setConfig(uint8_t packetType) {
    AACPacketType = packetType;
}

int FLVAudioTag::writeData(WriteStream &ws) const {
    ws.writeMultiBit(4, SoundFormat);
    ws.writeMultiBit(2, SoundRate);
    ws.writeMultiBit(1, SoundSize);
    ws.writeMultiBit(1, SoundType);
    ws.writeMultiBit(8, AACPacketType);

    return 0;
}

int FLVAudioTag::writeConfig(WriteStream &ws) const {
    ws.writeMultiBit(4, SoundFormat);
    ws.writeMultiBit(2, SoundRate);
    ws.writeMultiBit(1, SoundSize);
    ws.writeMultiBit(1, SoundType);
    ws.writeMultiBit(8, AACPacketType);

    /*--- aac config ---*/
    ws.writeMultiBit(5, audioObjectType);
    ws.writeMultiBit(4, samplingFrequencyIndex);
    ws.writeMultiBit(4, channelConfiguration);


    ws.writeMultiBit(1, frameLengthFlag);
    ws.writeMultiBit(1, dependsOnCoreCoder);
    ws.writeMultiBit(1, extensionFlag);
    return 0;
}

void FLVAudioTag::setConfigurationRecord(uint8_t profile, uint8_t index, uint8_t channel) {
    audioObjectType = profile;
    samplingFrequencyIndex = index;
    channelConfiguration = channel;
}


