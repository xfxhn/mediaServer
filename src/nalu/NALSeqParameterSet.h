
#ifndef MUX_NALSEQPARAMETERSET_H
#define MUX_NALSEQPARAMETERSET_H

// 7.4.2.1.1: num_ref_frames_in_pic_order_cnt_cycle的范围是0到255(包括0和255)。
#define H264_MAX_OFFSET_REF_FRAME_COUNT 256

#include <cstdint>

struct AVRational {
    int num;
    int den;
};

class ReadStream;

class NALSeqParameterSet {

public:
    uint8_t profile_idc{};
    uint8_t compatibility{0};
    bool constraint_set0_flag{};
    bool constraint_set1_flag{};
    bool constraint_set2_flag{};
    bool constraint_set3_flag{};
    bool constraint_set4_flag{};
    bool constraint_set5_flag{};

    uint8_t level_idc{};
    uint8_t seq_parameter_set_id{};
    uint8_t chroma_format_idc{};
    bool separate_colour_plane_flag{};
    uint8_t bit_depth_luma_minus8{};  //当bit_depth_luma_minus8不存在时，它将被推断为等于0。bit_depth_luma_minus8的取值范围为0到6(包括6)。
    uint8_t bit_depth_chroma_minus8{};
    bool qpprime_y_zero_transform_bypass_flag{false};  //没有特别指定时，应推定其值为0
    bool seq_scaling_matrix_present_flag{};
    /*bool seq_scaling_list_present_flag[12];*/
    uint8_t log2_max_frame_num_minus4{};  // 0-12
    uint16_t MaxFrameNum{0};
    uint8_t pic_order_cnt_type{};
    uint8_t log2_max_pic_order_cnt_lsb_minus4{};
    uint32_t MaxPicOrderCntLsb{0};
    bool delta_pic_order_always_zero_flag{true};
    int offset_for_non_ref_pic{0};  //-2^31-1 - 2^31-1，有符号4字节整数
    int offset_for_top_to_bottom_field{0};
    uint8_t num_ref_frames_in_pic_order_cnt_cycle{0}; // 0-255
    int offset_for_ref_frame[H264_MAX_OFFSET_REF_FRAME_COUNT]{}; //-2^31-1 - 2^31-1，有符号4字节整数
    int ExpectedDeltaPerPicOrderCntCycle{0};

    uint8_t max_num_ref_frames{};
    bool gaps_in_frame_num_value_allowed_flag{};
    uint32_t pic_width_in_mbs_minus1{};
    uint32_t pic_height_in_map_units_minus1{};
    bool frame_mbs_only_flag{};
    //bool mb_adaptive_frame_field_flag{};
    bool direct_8x8_inference_flag{};
    bool frame_cropping_flag{};
    uint32_t frame_crop_left_offset{};
    uint32_t frame_crop_right_offset{};
    uint32_t frame_crop_top_offset{};
    uint32_t frame_crop_bottom_offset{};


    uint32_t PicWidthInMbs{0};
    uint32_t PicHeightInMapUnits{0}; //PicHeightInMapUnits = pic_height_in_map_units_minus1 + 1;
    uint32_t PicSizeInMapUnits{0};
    uint8_t ChromaArrayType{0};
    uint32_t PicWidthInSamplesL{0};
    uint32_t PicHeightInSamplesL{0};

    bool vui_parameters_present_flag{false};


    //辅助信息

    double fps{25.0};
    AVRational timeBase{1, 25};

    uint8_t aspect_ratio_idc{0};
    //长宽比
    uint16_t sar_width{0};
    uint16_t sar_height{0};
    bool overscan_appropriate_flag{false};

    uint8_t video_format{5};
    bool video_full_range_flag{false};
    uint8_t colour_primaries{};
    uint8_t transfer_characteristics{};
    uint8_t matrix_coefficients{};

    //取值范围为0 ~ 5，包括0 ~ 5
    uint8_t chroma_sample_loc_type_top_field{};
    uint8_t chroma_sample_loc_type_bottom_field{};

    bool timing_info_present_flag{false};
    uint32_t num_units_in_tick{0};
    uint32_t time_scale{0};
    bool fixed_frame_rate_flag{false};


    bool low_delay_hrd_flag{false};


    int max_num_reorder_frames{-1};
    int max_dec_frame_buffering{-1};
public:
    int seq_parameter_set_data(ReadStream &rs);

    int vui_parameters(ReadStream &rs);
};


#endif //MUX_NALSEQPARAMETERSET_H
