#ifndef MUX_NALPICTURE_H
#define MUX_NALPICTURE_H

#include <cstdint>
#include <vector>

#include "NALSliceHeader.h"

struct Frame {
    uint32_t nalUintSize;
    uint8_t *data;
    int type;
};
enum PICTURE_MARKING {
    SHORT_TERM_REFERENCE, //短期参考帧
    LONG_TERM_REFERENCE,  //长期参考帧
    UNUSED_FOR_REFERENCE, //不用于参考
};

class NALPicture {

private:
    /*POC相关*/
    uint32_t PicOrderCntMsb{0};
    uint32_t TopFieldOrderCnt{0};
    uint32_t BottomFieldOrderCnt{0};
    int FrameNumOffset{0};

public:
    /*记录这帧大小*/
    uint32_t size{0};

    uint64_t dts{0};
    uint64_t pts{0};
    uint64_t pcr{0};
    double duration{0};

    uint32_t pictureOrderCount{0};
    /*前一图像（按照解码顺序）*/
    NALPicture *previousPicture{nullptr};
    /*前一个参考图像（按照解码顺序）*/
    NALPicture *referencePicture{nullptr};

    /*表示当前这帧是否正在使用*/
    bool useFlag{false};

    PICTURE_MARKING reference_marked_type{UNUSED_FOR_REFERENCE};
    int MaxLongTermFrameIdx{-1}; //长期参考帧的最大数目
    int LongTermFrameIdx{-1}; //长期参考帧索引


    /*等于frame_num*/
    uint16_t FrameNum{0};
    int FrameNumWrap{-1};
    /*标记一个短期参考图像*/
    int PicNum{-1};
    /*标记一个长期参考图像*/
    int LongTermPicNum{-1};


    NALSliceHeader sliceHeader;
    std::vector<Frame> data;


    bool memory_management_control_operation_5_flag{false};
    bool memory_management_control_operation_6_flag{false};
public:
    void reset();

    int decoding_process_for_picture_order_count();

    int decoding_process_for_picture_order_count_type_0();

    int decoding_process_for_picture_order_count_type_1();

    int decoding_process_for_picture_order_count_type_2();

    ~NALPicture();
};


#endif //MUX_NALPICTURE_H
