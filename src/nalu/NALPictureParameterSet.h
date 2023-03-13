#ifndef MUX_NALPICTUREPARAMETERSET_H
#define MUX_NALPICTUREPARAMETERSET_H


#include <cstdint>

class ReadStream;

class NALSeqParameterSet;

class NALPictureParameterSet {
public:
    uint8_t pic_parameter_set_id{0};
    uint8_t seq_parameter_set_id{0};
    uint8_t entropy_coding_mode_flag{0};
    bool bottom_field_pic_order_in_frame_present_flag{false};

    /*片组相关的参数*/
    uint8_t num_slice_groups_minus1{0};
    uint8_t slice_group_map_type{0};
    uint32_t run_length_minus1[8]{0};
    uint32_t top_left[8]{0};
    uint32_t bottom_right[8]{0};
    uint8_t slice_group_change_direction_flag{0};
    uint32_t slice_group_change_rate_minus1{0};
    uint32_t pic_size_in_map_units_minus1{0};
    uint8_t *slice_group_id{nullptr};


    /*值应该在0到31的范围内(含31)*/
    uint8_t num_ref_idx_l0_default_active_minus1{0};
    uint8_t num_ref_idx_l1_default_active_minus1{0};

    uint8_t weighted_pred_flag{0};
    uint8_t weighted_bipred_idc{0};
    /*值范围为−(26 + QpBdOffsetY) ~ +25(含)*/
    int8_t pic_init_qp_minus26{0};
    /*值范围为−26 ~ +25(含)*/
    int8_t pic_init_qs_minus26{0};
    int8_t chroma_qp_index_offset{0};
    uint8_t deblocking_filter_control_present_flag{0};
    uint8_t constrained_intra_pred_flag{0};
    uint8_t redundant_pic_cnt_present_flag{0};
    int8_t second_chroma_qp_index_offset{0};

public:
    int pic_parameter_set_rbsp(ReadStream &rs, const NALSeqParameterSet (&spsList)[32]);

    NALPictureParameterSet& operator=(const NALPictureParameterSet &pps);

    ~NALPictureParameterSet();
};


#endif //MUX_NALPICTUREPARAMETERSET_H
