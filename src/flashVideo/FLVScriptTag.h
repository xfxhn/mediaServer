
#ifndef MEDIASERVER_SCRIPTTAG_H
#define MEDIASERVER_SCRIPTTAG_H

#include <string>
#include "writeStream.h"


enum marker {
    number_marker,
    boolean_marker,
    string_marker = 2,
    object_marker,
    movieclip_marker,
    null_marker,
    undefined_marker,
    reference_marker,
    ecma_array_marker,
    object_end_marker
};
struct Item {
    std::string name;
    marker type;
};

class FLVScriptTag {
private:
    double videocodecid{7.0};
    double audiocodecid{10.0};


    double width{0};
    double height{0};
    double framerate{0};
    double audiosamplerate{0};
    bool stereo{false};
public:
    void setConfig(double _width, double _height, double _fps, double _sampleRate, uint8_t channel);

    int write(WriteStream &ws);
};


#endif //MEDIASERVER_SCRIPTTAG_H
