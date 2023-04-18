
#include "NALSliceHeader.h"

#include <cstdio>


#include "NALHeader.h"


int NALSliceHeader::slice_header(ReadStream &rs, NALHeader &nalUnit, const NALSeqParameterSet spsList[],
                                 const NALPictureParameterSet ppsList[]) {

    int ret;

    first_mb_in_slice = rs.readUE();
    slice_type = rs.readUE() % 5;
    pic_parameter_set_id = rs.readUE();

    nalu = nalUnit;
    pps = ppsList[pic_parameter_set_id];
    sps = spsList[pps.seq_parameter_set_id];


    if (sps.separate_colour_plane_flag) {
        //指定与当前切片RBSP关联的颜色平面。colour_plane_id的值应该在0到2的范围内
        //colour_plane_id等于0、1和2分别对应于Y、Cb和Cr平面。
        colour_plane_id = rs.readMultiBit(2); //2 u(2)
    }

    frame_num = rs.readMultiBit(sps.log2_max_frame_num_minus4 + 4);

    if (nalu.IdrPicFlag) {
        idr_pic_id = rs.readUE();
    }

    if (sps.pic_order_cnt_type == 0) {
        pic_order_cnt_lsb = rs.readMultiBit(sps.log2_max_pic_order_cnt_lsb_minus4 + 4);

        //当field_pic_flag 不存在时，应推定其值为0
        if (pps.bottom_field_pic_order_in_frame_present_flag) {
            //表示一个编码帧的底场和顶场的图像顺序数之间的差 不存在时推断为0
            delta_pic_order_cnt_bottom = rs.readSE();
        }
    }

    if (sps.pic_order_cnt_type == 1 && !sps.delta_pic_order_always_zero_flag) {
        /*
         * POC 的第二和第三种算法是从 frame_num 映射得来，这两个句法元素用于映射算法。
         * delta_pic_order_cnt[0]用于帧编码方式下的底场和场编码方式的场，
         * delta_pic_order_cnt[1]用于帧编码方式下的顶场
         * */
        delta_pic_order_cnt[0] = rs.readSE();
        if (pps.bottom_field_pic_order_in_frame_present_flag) {
            delta_pic_order_cnt[1] = rs.readSE();
        }
    }

/*对于那些属于基本编码图像的条带和条带数据分割块应等于0。在冗余编码图像中的编码
条带和编码条带数据分割块的 redundant_pic_cnt 的值应大于 0。当redundant_pic_cnt不存在时，默认其值为 0。
redundant_pic_cnt的值应该在 0 到 127 范围内（包括 0 和 127）*/
    if (pps.redundant_pic_cnt_present_flag) {
        redundant_pic_cnt = rs.readUE();
    }

    if (slice_type == H264_SLIECE_TYPE_B) {
        //指出在B图像的直接预测的模式下，用时间预测还是用空间预测。1： 空间预测；0：时间预测。
        direct_spatial_mv_pred_flag = rs.readBit();
    }

    if (slice_type == H264_SLIECE_TYPE_P || slice_type == H264_SLIECE_TYPE_SP || slice_type == H264_SLIECE_TYPE_B) {
        /*
            num_ref_idx_l0_active_minus1 和 num_ref_idx_l1_active_minus1 指定当前参考帧队列中实际可用的参考帧的数目。
            在片头可以重载这对句法元素，以给某特定图像更大的灵活度。
            这个句法元素就是指明片头是否会重载，如果该句法元素等于 1，
            下面会出现新的num_ref_idx_l0_active_minus1和num_ref_idx_l1_active_minus1值*/
        bool num_ref_idx_active_override_flag = rs.readBit();

        num_ref_idx_l0_active_minus1 = pps.num_ref_idx_l0_default_active_minus1;
        num_ref_idx_l1_active_minus1 = pps.num_ref_idx_l1_default_active_minus1;
        if (num_ref_idx_active_override_flag) {
            num_ref_idx_l0_active_minus1 = rs.readUE();
            if (slice_type == H264_SLIECE_TYPE_B) {
                num_ref_idx_l1_active_minus1 = rs.readUE();
            }
        }
    }

    if (nalu.nal_unit_type == 20 || nalu.nal_unit_type == 21) {
        fprintf(stderr, "参见附录H,不支持解析\n");
        return -1;
    } else {
        /* 参考图像序列重排序相关 */
        ret = ref_pic_list_modification(rs);
        if (ret < 0) {
            fprintf(stderr, "slice header 解析参考帧列表排序字段失败\n");
            return ret;
        }
    }


    if ((pps.weighted_pred_flag && (slice_type == H264_SLIECE_TYPE_P || slice_type == H264_SLIECE_TYPE_SP)) ||
        (pps.weighted_bipred_idc == 1 && slice_type == H264_SLIECE_TYPE_B)
            ) {
        /*加权预测相关*/
        pred_weight_table(rs);
    }

    if (nalu.nal_ref_idc != 0) {
        /*参考图像序列标记相关*/
        dec_ref_pic_marking(rs);
    }
    // todo...



    /*MaxPicNum = (field_pic_flag == 0) ? sps.MaxFrameNum : (2 * sps.MaxFrameNum);
    CurrPicNum = (field_pic_flag == 0) ? frame_num : (2 * frame_num + 1);*/
    /*表征 PicNum 的最大值，PicNum 和 frame_num 一样，也是嵌在循环中，当达到这个最大值时，PicNum 将从 0 开始重新计数*/
    MaxPicNum = sps.MaxFrameNum;
    /*当前图像的 PicNum 值，在计算 PicNum 的过程中，当前图像的 PicNum 值是由 frame_num 直接算出*/
    CurrPicNum = frame_num;
    return 0;
}

int NALSliceHeader::ref_pic_list_modification(ReadStream &rs) {
    //H264_SLIECE_TYPE_I = 2; H264_SLIECE_TYPE_SI = 4;
    if (slice_type != 2 && slice_type != 4) {
        bool ref_pic_list_modification_flag_l0 = rs.readBit();
        /*指明了是否有重排序的操作*/
        if (ref_pic_list_modification_flag_l0) {
            /*指定参考图片列表0*/
            int i = 0;
            do {
                if (i >= 32) {
                    fprintf(stderr, "参考列表0，i必须在0-31\n");
                    return -1;
                }

                modification_of_pic_nums_idc[0][i] = rs.readUE();
                if (modification_of_pic_nums_idc[0][i] == 0 || modification_of_pic_nums_idc[0][i] == 1) {
                    /*在对短期参考帧重排序时指明重排序图像与当前的差*/
                    abs_diff_pic_num_minus1[0][i] = rs.readUE();
                } else if (modification_of_pic_nums_idc[0][i] == 2) {
                    /*在对长期参考帧重排序时指明重排序图像*/
                    long_term_pic_num[0][i] = rs.readUE();
                }
                i++;
            } while (modification_of_pic_nums_idc[0][i - 1] != 3);
            // ref_pic_list_modification_count_l0 = i;
        }
    }

    //H264_SLIECE_TYPE_B = 1;
    if (slice_type % 5 == 1) {
        bool ref_pic_list_modification_flag_l1 = rs.readBit();
        /*指明了是否有重排序的操作*/
        if (ref_pic_list_modification_flag_l1) {
            int i = 0;
            do {
                if (i >= 32) {
                    fprintf(stderr, "参考列表1，i必须在0-31\n");
                    return -1;
                }

                modification_of_pic_nums_idc[1][i] = rs.readUE();
                if (modification_of_pic_nums_idc[1][i] == 0 || modification_of_pic_nums_idc[1][i] == 1) {
                    abs_diff_pic_num_minus1[1][i] = rs.readUE();
                } else if (modification_of_pic_nums_idc[1][i] == 2) {
                    long_term_pic_num[1][i] = rs.readUE();
                }
                i++;
            } while (modification_of_pic_nums_idc[1][i - 1] != 3);
            //ref_pic_list_modification_count_l1 = i;
        }
    }
    return 0;
}

int NALSliceHeader::pred_weight_table(ReadStream &rs) {

    int i = 0;
    int j = 0;

    luma_log2_weight_denom = rs.readUE();
    if (sps.ChromaArrayType != 0) {
        chroma_log2_weight_denom = rs.readUE();
    }

    for (i = 0; i <= num_ref_idx_l0_active_minus1; i++) {
        /*luma_log2_weight_denom是所有luma权重因子的分母以2为底的对数*/
        luma_weight_l0[i] = static_cast<int8_t>(1 << luma_log2_weight_denom); //When luma_weight_l0_flag=0
        luma_offset_l0[i] = 0; //When luma_weight_l0_flag is equal to 0
        /*等于1时，指的是在参考序列0中的亮度的加权系数存在；等于0时，在参考序列0中的亮度的加权系数不存在*/
        bool luma_weight_l0_flag = rs.readBit();
        if (luma_weight_l0_flag) {
            luma_weight_l0[i] = static_cast<int8_t>(rs.readSE());
            luma_offset_l0[i] = static_cast<int8_t>(rs.readSE());
        }

        if (sps.ChromaArrayType != 0) {
            /*chroma_log2_weight_denom是所有chroma权重因子的分母以2为底的对数*/
            chroma_weight_l0[i][0] = static_cast<int8_t>(1 << chroma_log2_weight_denom); //When chroma_weight_l0_flag=0
            chroma_weight_l0[i][1] = static_cast<int8_t>(1 << chroma_log2_weight_denom); //When chroma_weight_l0_flag=0
            chroma_offset_l0[i][0] = 0;
            chroma_offset_l0[i][1] = 0;

            /*chroma_weight_l0_flag=0，色度权重因子不存在*/
            bool chroma_weight_l0_flag = rs.readBit();

            /*chroma_weight_l0[i][j]是应用于使用RefPicList0[i]进行列表0预测的色度预测值的权重因子,取值范围为−128 ~ 127*/
            /*chroma_offset_l0[i][j]是使用refpiclist0[i]对列表0预测的色度预测值应用的加性偏移量,取值范围为−128 ~ 127*/
            /*对于Cb j等于0，对于Cr j等于1*/
            if (chroma_weight_l0_flag) {
                for (j = 0; j < 2; j++) {
                    chroma_weight_l0[i][j] = static_cast<int8_t>(rs.readSE());
                    chroma_offset_l0[i][j] = static_cast<int8_t>(rs.readSE());
                }
            }
        }
    }

    if (slice_type % 5 == 1) {
        for (i = 0; i <= num_ref_idx_l1_active_minus1; i++) {
            luma_weight_l1[i] = static_cast<int8_t>(1 << luma_log2_weight_denom); //When luma_weight_l1_flag=0
            luma_offset_l1[i] = 0; //When luma_weight_l1_flag=0

            bool luma_weight_l1_flag = rs.readBit();
            if (luma_weight_l1_flag) {
                luma_weight_l1[i] = static_cast<int8_t>(rs.readSE());
                luma_offset_l1[i] = static_cast<int8_t>(rs.readSE());
            }

            if (sps.ChromaArrayType != 0) {
                chroma_weight_l1[i][0] = static_cast<int8_t>(1 << chroma_log2_weight_denom); //chroma_weight_l1_flag=0
                chroma_weight_l1[i][1] = static_cast<int8_t>(1 << chroma_log2_weight_denom); //chroma_weight_l1_flag=0
                chroma_offset_l1[i][0] = 0;
                chroma_offset_l1[i][1] = 0;

                bool chroma_weight_l1_flag = rs.readBit();
                if (chroma_weight_l1_flag) {
                    for (j = 0; j < 2; j++) {
                        chroma_weight_l1[i][j] = static_cast<int8_t>(rs.readSE());
                        chroma_offset_l1[i][j] = static_cast<int8_t>(rs.readSE());
                    }
                }
            }
        }
    }
    return 0;
}

int NALSliceHeader::dec_ref_pic_marking(ReadStream &rs) {
    if (nalu.IdrPicFlag) {
        no_output_of_prior_pics_flag = rs.readBit();
        long_term_reference_flag = rs.readBit();
    } else {
        /*=0,先入先出（FIFO）：使用滑动窗的机制，先入先出，在这种模式下没有办法对长期参考帧进行操作*/
        /*=1,自适应标记（marking）：后续码流中会有一系列句法元素显式指明操作的步骤。自适应是指编码器可根据情况随机灵活地作出决策。*/
        adaptive_ref_pic_marking_mode_flag = rs.readBit();
        if (adaptive_ref_pic_marking_mode_flag) {
            int i = 0;
            do {
                dec_ref_pic_markings[i].memory_management_control_operation = rs.readUE();

                if (dec_ref_pic_markings[i].memory_management_control_operation == 1 ||
                    dec_ref_pic_markings[i].memory_management_control_operation == 3) {
                    // 这个句法元素可以计算得到需要操作的图像在短期参考队列中的序号
                    dec_ref_pic_markings[i].difference_of_pic_nums_minus1 = rs.readUE();
                }

                if (dec_ref_pic_markings[i].memory_management_control_operation == 2) {
                    //  此句法元素得到所要操作的长期参考图像的序号
                    dec_ref_pic_markings[i].long_term_pic_num = rs.readUE();
                }

                if (dec_ref_pic_markings[i].memory_management_control_operation == 3 ||
                    dec_ref_pic_markings[i].memory_management_control_operation == 6) {
                    // 分配一个长期参考帧的序号给一个图像
                    dec_ref_pic_markings[i].long_term_frame_idx = rs.readUE();
                }

                if (dec_ref_pic_markings[i].memory_management_control_operation == 4) {
                    //此句法元素减1 ，指明长期参考队列的最大数目。
                    dec_ref_pic_markings[i].max_long_term_frame_idx_plus1 = rs.readUE();
                }
                ++i;

            } while (dec_ref_pic_markings[i - 1].memory_management_control_operation != 0);
            dec_ref_pic_markings_size = i;
        }
    }

    return 0;
}

