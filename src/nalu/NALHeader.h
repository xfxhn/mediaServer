
#ifndef MUX_NALHEADER_H
#define MUX_NALHEADER_H

#include <cstdint>


class NALHeader {
public:
    bool forbidden_zero_bit{false};
    /*用来指示当前NAL单元的优先级。0的优先级最低，
     * 如果当前NAL单元内是非参考图像的slice或者slice data partition等不是那么重要的数据，那么这个值为0；
     * 如果当前NAL单元内是sps、pps、参考图像的slice或者slice data partition等重要数据，那么这个值不为0*/
    uint8_t nal_ref_idc{0};
    uint8_t nal_unit_type{5};
    bool IdrPicFlag{false};
public:
    void nal_unit(uint8_t header);

    static void ebsp_to_rbsp(uint8_t *data, uint32_t &size);

};


#endif //MUX_NALHEADER_H
