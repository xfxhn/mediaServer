#include "audioSpecificConfig.h"

#include "writeStream.h"


int AudioSpecificConfig::write(WriteStream &ws) const {

    ws.writeMultiBit(5, audioObjectType);
    ws.writeMultiBit(4, samplingFrequencyIndex);
    ws.writeMultiBit(4, channelConfiguration);


    ws.writeMultiBit(1, frameLengthFlag);
    ws.writeMultiBit(1, dependsOnCoreCoder);
    ws.writeMultiBit(1, extensionFlag);

    return 0;
}

int AudioSpecificConfig::setConfig(uint8_t type, uint8_t index, uint8_t channel) {
    if (type == 5 || type == 29) {
        fprintf(stderr, "不支持写入 audioObjectType=5，或者audioObjectType=29");
        return -1;
    }
    audioObjectType = type;
    samplingFrequencyIndex = index;
    channelConfiguration = channel;
    return 0;

}








