
#ifndef MUX_NALSLICEHEADER_H
#define MUX_NALSLICEHEADER_H

#include <cstdint>
#include "bitStream/readStream.h"
#include "NALHeader.h"
#include "NALSeqParameterSet.h"
#include "NALPictureParameterSet.h"


class NALSeqParameterSet;

class NALPictureParameterSet;

enum SLIECETYPE {
    H264_SLIECE_TYPE_P = 0,
    H264_SLIECE_TYPE_B = 1,
    H264_SLIECE_TYPE_I = 2,
    H264_SLIECE_TYPE_SP = 3,
    H264_SLIECE_TYPE_SI = 4,
    H264_SLIECE_TYPE_P2 = 5,
    H264_SLIECE_TYPE_B2 = 6,
    H264_SLIECE_TYPE_I2 = 7,
    H264_SLIECE_TYPE_SP2 = 8,
    H264_SLIECE_TYPE_SI2 = 9,
};
struct DEC_REF_PIC_MARKING {
    uint8_t memory_management_control_operation; //在自适应标记（marking）模式中指明本次操作的具体内容 表7-24
    uint8_t difference_of_pic_nums_minus1;
    uint8_t long_term_pic_num;
    uint8_t long_term_frame_idx;
    uint8_t max_long_term_frame_idx_plus1;
};

class NALSliceHeader {
public:
    NALHeader nalu;
    NALSeqParameterSet sps;
    NALPictureParameterSet pps;

    uint32_t first_mb_in_slice{0};
    uint8_t slice_type{0};
    uint8_t pic_parameter_set_id{0};
    uint8_t colour_plane_id{0};
    uint16_t frame_num{0};
    uint16_t idr_pic_id{0};
    uint16_t pic_order_cnt_lsb{0};
    int delta_pic_order_cnt_bottom{0};
    /*默认值为0*/
    int delta_pic_order_cnt[2]{0};


    uint8_t redundant_pic_cnt{0};
    bool direct_spatial_mv_pred_flag{false};
    uint8_t num_ref_idx_l0_active_minus1{0};
    uint8_t num_ref_idx_l1_active_minus1{0};

    uint8_t modification_of_pic_nums_idc[2][32]{0};
    uint16_t abs_diff_pic_num_minus1[2][32]{0}; // 0 到 MaxPicNum – 1
    uint32_t long_term_pic_num[2][32];

    /*加权预测*/
    /*给出参考帧列表中参考图像所有亮度的加权系数，是个初始值uma_log2_weight_denom 值的范围是 0 to 7*/
    uint8_t luma_log2_weight_denom{0};
    /*给出参考帧列表中参考图像所有色度的加权系数，是个初始值chroma_log2_weight_denom 值的范围是  0 to 7*/
    uint8_t chroma_log2_weight_denom{0};
    /*用参考序列 0 预测亮度值时，所用的加权系数*/
    int8_t luma_weight_l0[32]{0};
    /*用参考序列 0 预测亮度值时，所用的加权系数的额外的偏移 。luma_offset_l0[ i ] 值的范围–128 to 127*/
    int8_t luma_offset_l0[32]{0};
    int8_t chroma_weight_l0[32][2]{0};
    int8_t chroma_offset_l0[32][2]{0};

    /*参考列表1的权重因子*/
    int8_t luma_weight_l1[32]{0};
    int8_t luma_offset_l1[32]{0};
    int8_t chroma_weight_l1[32][2]{0};
    int8_t chroma_offset_l1[32][2]{0};

    /*参考图像序列标记相关*/
    /*指明是否要将前面已解码的图像全部输出*/
    bool no_output_of_prior_pics_flag{false};
    /*
     * 这个句法元素指明是否使用长期参考这个机制。
     * 如果取值为 1，表明使用长期参考，并且每个 IDR 图像被解码后自动成为长期参考帧，
     * 否则（取值为 0），IDR 图像被解码后自动成为短期参考帧
     * */
    bool long_term_reference_flag{false};
    /*指明标记（marking）操作的模式*/
    bool adaptive_ref_pic_marking_mode_flag{false};
    /*在自适应标记（marking）模式中，指明本次操作的具体内容 见毕厚杰表 7.24*/
    uint8_t memory_management_control_operation{0};
    DEC_REF_PIC_MARKING dec_ref_pic_markings[32];
    /*dec_ref_pic_markings实际大小*/
    uint8_t dec_ref_pic_markings_size{0};

    uint16_t MaxPicNum{0};
    uint16_t CurrPicNum{0};
public:
    int slice_header(ReadStream &rs, NALHeader &nalUnit, const NALSeqParameterSet spsList[],
                     const NALPictureParameterSet ppsList[]);

private:
    int ref_pic_list_modification(ReadStream &rs);

    int pred_weight_table(ReadStream &rs);

    int dec_ref_pic_marking(ReadStream &rs);
};


#endif //MUX_NALSLICEHEADER_H
