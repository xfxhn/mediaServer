
#include "FLVScriptTag.h"
#include <cstring>

static Item list[7] = {
        {"width",           number_marker},
        {"height",          number_marker},
        {"framerate",       number_marker},
        {"videocodecid",    number_marker},
        {"audiosamplerate", number_marker},
        {"stereo",          boolean_marker},
        {"audiocodecid",    number_marker}
};

int FLVScriptTag::write(WriteStream &ws) {

    /*第⼀个AMF包*/
    ws.writeMultiBit(8, string_marker);
    const char *msg = "onMetaData";
    ws.writeMultiBit(16, strlen(msg));
    ws.setString(msg, strlen(msg));

    /*第⼆个AMF包*/
    ws.writeMultiBit(8, ecma_array_marker);
    /*数组的长度*/
    ws.writeMultiBit(32, 7);


    for (auto &i: list) {
        /*写入字符串大小*/
        ws.writeMultiBit(16, i.name.length());
        /*写入字符串*/
        ws.setString(i.name.c_str(), i.name.length());
        ws.writeMultiBit(8, i.type);

        if (i.type == number_marker) {
            if (i.name == "width") {
                ws.writeMultiBit(64, *(uint64_t *) (&width));
            } else if (i.name == "height") {
                ws.writeMultiBit(64, *(uint64_t *) (&height));
            } else if (i.name == "framerate") {
                ws.writeMultiBit(64, *(uint64_t *) (&framerate));
            } else if (i.name == "videocodecid") {
                ws.writeMultiBit(64, *(uint64_t *) (&videocodecid));
            } else if (i.name == "audiosamplerate") {
                ws.writeMultiBit(64, *(uint64_t *) (&audiosamplerate));
            } else if (i.name == "audiocodecid") {
                ws.writeMultiBit(64, *(uint64_t *) (&audiocodecid));
            }

        } else if (i.type == boolean_marker) {
            ws.writeMultiBit(8, stereo);
        } else {
            fprintf(stderr, "AMF 数组写入不对\n");
            return -1;
        }
    }


    /*输入结束符*/
    ws.writeMultiBit(24, object_end_marker);
    return 0;
}

void FLVScriptTag::setConfig(double _width, double _height, double _fps, double _sampleRate, uint8_t channel) {
    width = _width;
    height = _height;
    framerate = _fps;
    audiosamplerate = _sampleRate;
    stereo = channel == 2;
}
