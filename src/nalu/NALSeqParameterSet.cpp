
#include "NALSeqParameterSet.h"

#include <cmath>
#include <cstdio>



/*参考table A-1*/
struct levelLimits {
    int levelNumber;
    //最大宏块处理速率
    int MaxMBPS;
    //最大帧尺寸
    int MaxFS;
    //最大解码图片缓冲区大小
    int MaxDpbMbs;
    //最大视频比特率
    int MaxBR;
//    //最大 CPB 尺寸
//    int MaxCPB;
//    //垂直MV分量极限
//    int MaxVmvR;
//    //最小压缩率
//    int MinCR;
//    //每两个连续 MB 的 MV 最大数
//    int MaxMvsPer2Mb;
};

static levelLimits levelTable[] = {
        {10, 1485,     99,     396,    64},
        {11, 3000,     396,    900,    192},
        {12, 6000,     396,    2376,   384},
        {13, 11880,    396,    2376,   768},
        {20, 11880,    396,    2376,   2000},
        {21, 19800,    792,    4752,   4000},
        {22, 20250,    1620,   8100,   4000},
        {30, 40500,    1620,   8100,   1000},
        {31, 108000,   3600,   18000,  14000},
        {32, 216000,   5120,   20480,  20000},
        {40, 245760,   8192,   32768,  20000},
        {41, 245760,   8192,   32768,  50000},
        {42, 522240,   8704,   34816,  50000},
        {50, 589824,   22080,  110400, 135000},
        {51, 983040,   36864,  184320, 240000},
        {52, 2073600,  36864,  184320, 240000},
        {60, 4177920,  139264, 696320, 240000},
        {61, 8355840,  139264, 696320, 480000},
        {62, 16711680, 139264, 696320, 800000},
};

int NALSeqParameterSet::seq_parameter_set_data(ReadStream &rs) {
    int ret = 0;
    //编码等级
    /*
    66	Baseline
    77	Main
    88	Extended
    100	High(FRExt)
    110	High 10 (FRExt)
    122	High 4:2 : 2 (FRExt)
    144	High 4 : 4 : 4 (FRExt)*/
    profile_idc = rs.readMultiBit(8);

    compatibility = rs.getMultiBit(8);
    constraint_set0_flag = rs.readMultiBit(1);
    constraint_set1_flag = rs.readMultiBit(1);
    constraint_set2_flag = rs.readMultiBit(1);
    constraint_set3_flag = rs.readMultiBit(1);
    constraint_set4_flag = rs.readMultiBit(1);
    constraint_set5_flag = rs.readMultiBit(1);
    rs.readMultiBit(2); //reserved_zero_2bits /* equal to 0 */
    level_idc = rs.readMultiBit(8); //level_idc 0 u(8)
    //要解码一个 Slice，需要有 SPS 和 PPS
    //但是码流中会有很多个SPS和PPS
    //如何确定依赖的是哪个SPS和PPS呢？
    //那就给 SPS 和 PPS 设置一个 ID 就行了
    seq_parameter_set_id = rs.readUE();

    //chroma_format_idc = 0	单色
    //chroma_format_idc = 1	YUV 4 : 2 : 0
    //chroma_format_idc = 2	YUV 4 : 2 : 2
    //chroma_format_idc = 3	YUV 4 : 4 : 4
    //当chroma_format_idc不存在时，它将被推断为等于1(4:2:0色度格式)。
    //码流是什么格式
    chroma_format_idc = 1;
    //只有当 profile_idc 等于这些值的时候，chroma_format_idc 才会被显式记录，
    //那么如果 profile_idc 不是这些值，chroma_format_idc 就会取默认值。
    if (profile_idc == 100 //A.2.4 High profile //Only I, P, and B slice types may be present.
        || profile_idc == 110 //A.2.5(A.2.8) High 10 (Intra) profile //Only I, P, and B slice types may be present.
        || profile_idc == 122 //A.2.6(A.2.9) High 4:2:2 (Intra) profile //Only I, P, and B slice types may be present.
        || profile_idc ==
           244 //A.2.7(A.2.10) High 4:4:4 Predictive/Intra profile //Only I, P, B slice types may be present.
        || profile_idc == 44 //A.2.11 CAVLC 4:4:4 Intra profile
        || profile_idc ==
           83 //G.10.1.2.1 Scalable Constrained High profile (SVC) //Only I, P, EI, and EP slices shall be present.
        || profile_idc == 86 //Scalable High Intra profile (SVC)
        || profile_idc == 118 //Stereo High profile (MVC)
        || profile_idc == 128 //Multiview High profile (MVC)
        || profile_idc == 138 //Multiview Depth High profile (MVCD)
        || profile_idc == 139 //
        || profile_idc == 134 //
        || profile_idc == 135 //
            ) {
        chroma_format_idc = rs.readUE();
        //当在码流中读取到 chroma_format_idc 之后separate_colour_plane_flag
        //当separate_colour_plane_flag不存在时，它将被推断为等于0。
        separate_colour_plane_flag = false;
        //这个语法元素在 chroma_format_idc 等于 3，也就是 YUV 444 模式的时候才有
        if (chroma_format_idc == 3) {
            /*当图像是 YUV 444 的时候，YUV 三个分量的比重是相同的，那么就有两种编码方式了。
            第一种，就是和其他格式一样，让 UV 分量依附在 Y 分量上；第二种，就是把 UV 和 Y 分开，独立出来。
            separate_colour_plane_flag 这个值默认是 0，表示 UV 依附于 Y，和 Y 一起编码，
            如果 separate_colour_plane_flag 变成 1，则表示 UV 与 Y 分开编码。
            而对于分开编码的模式，我们采用和单色模式一样的规则。*/
            separate_colour_plane_flag = rs.readBit();
        }
        //是指亮度队列样值的比特深度以及亮度量化参数范围的取值偏移    QpBdOffsetY
        bit_depth_luma_minus8 = rs.readUE();
        //与bit_depth_luma_minus8类似，只不过是针对色度的
        bit_depth_chroma_minus8 = rs.readUE();

        //等于1是指当 QP'Y 等于 0 时变换系数解码过程的变换旁路操作和图 像构建过程将会在第8.5 节给出的去块效应滤波过程之前执行。
        //qpprime_y_zero_transform_bypass_flag 等于0 是指变换系数解码过程和图像构建过程在去块效应滤波过程之前执行而不使用变换旁路操作。
        //当 qpprime_y_zero_transform_bypass_flag 没有特别指定时，应推定其值为0。
        qpprime_y_zero_transform_bypass_flag = rs.readBit();
        //缩放标志位
        seq_scaling_matrix_present_flag = rs.readBit();


        //const size_t length = (this->chroma_format_idc != 3) ? 8 : 12;
        // TODO
        /*if (this->seq_scaling_matrix_present_flag) {
            constexpr int Default_4x4_Intra[16] = {6, 13, 13, 20, 20, 20, 28, 28, 28, 28, 32, 32, 32, 37, 37, 42};
            constexpr int Default_4x4_Inter[16] = {10, 14, 14, 20, 20, 20, 24, 24, 24, 24, 27, 27, 27, 30, 30, 34};

            constexpr int Default_8x8_Intra[64] =
                    {
                            6, 10, 10, 13, 11, 13, 16, 16, 16, 16, 18, 18, 18, 18, 18, 23,
                            23, 23, 23, 23, 23, 25, 25, 25, 25, 25, 25, 25, 27, 27, 27, 27,
                            27, 27, 27, 27, 29, 29, 29, 29, 29, 29, 29, 31, 31, 31, 31, 31,
                            31, 33, 33, 33, 33, 33, 36, 36, 36, 36, 38, 38, 38, 40, 40, 42,
                    };

            constexpr int Default_8x8_Inter[64] =
                    {
                            9, 13, 13, 15, 13, 15, 17, 17, 17, 17, 19, 19, 19, 19, 19, 21,
                            21, 21, 21, 21, 21, 22, 22, 22, 22, 22, 22, 22, 24, 24, 24, 24,
                            24, 24, 24, 24, 25, 25, 25, 25, 25, 25, 25, 27, 27, 27, 27, 27,
                            27, 28, 28, 28, 28, 28, 30, 30, 30, 30, 32, 32, 32, 33, 33, 35,
                    };
            for (size_t i = 0; i < length; i++) {
                this->seq_scaling_list_present_flag[i] = rs.readBit(); //seq_scaling_list_present_flag[ i ] 0 u(1)
                if (this->seq_scaling_list_present_flag[i]) {
                    if (i < 6) {
                        scaling_list(bs, ScalingList4x4[i], 16, UseDefaultScalingMatrix4x4Flag[i]);

                        //当计算出 useDefaultScalingMatrixFlag 等于 1 时，应推定缩放比例列表等于表 7 - 2 给出的默认的缩放比例列 表。
                        if (UseDefaultScalingMatrix4x4Flag[i]) {
                            if (i == 0 || i == 1 || i == 2) {
                                memcpy(ScalingList4x4[i], Default_4x4_Intra, sizeof(int) * 16);
                            } else { //if (i >= 3)
                                memcpy(ScalingList4x4[i], Default_4x4_Inter, sizeof(int) * 16);
                            }
                        }
                    } else {
                        scaling_list(rs, ScalingList8x8[i - 6], 64, UseDefaultScalingMatrix8x8Flag[i - 6]);

                        //当计算出 useDefaultScalingMatrixFlag 等于 1 时，应推定缩放比例列表等于表 7 - 2 给出的默认的缩放比例列 表。
                        if (UseDefaultScalingMatrix8x8Flag[i - 6]) {
                            if (i == 6 || i == 8 || i == 10) {
                                memcpy(ScalingList8x8[i - 6], Default_8x8_Intra, sizeof(int) * 64);
                            } else {
                                memcpy(ScalingList8x8[i - 6], Default_8x8_Inter, sizeof(int) * 64);
                            }
                        }
                    }
                } else//视频序列参数集中不存在缩放比例列表
                {
                    //表7-2的规定 集 A
                    if (i < 6) {
                        if (i == 0 || i == 1 || i == 2) {
                            memcpy(ScalingList4x4[i], Default_4x4_Intra, sizeof(int) * 16);
                        } else {
                            memcpy(ScalingList4x4[i], Default_4x4_Inter, sizeof(int) * 16);
                        }
                    } else {
                        if (i == 6 || i == 8 || i == 10) {
                            memcpy(ScalingList8x8[i - 6], Default_8x8_Intra, sizeof(int) * 64);
                        } else if (i == 7 || i == 9 || i == 11) {
                            memcpy(ScalingList8x8[i - 6], Default_8x8_Inter, sizeof(int) * 64);
                        }
                    }
                }

            }
        }*/


    }

    //这个句法元素主要是为读取另一个句法元素 frame_num 服务的，frame_num 是最重要的句法元素之一，它标识所属图像的解码顺序 。
    //这个句法元素同时也指明了frame_num 的所能达到的最大值:MaxFrameNum = 2*exp( log2_max_frame_num_minus4 + 4 )
    //最大帧率
    log2_max_frame_num_minus4 = rs.readUE();//0 - 12

    MaxFrameNum = static_cast<uint16_t> (std::pow(2, log2_max_frame_num_minus4 + 4));
    //指明了 poc  (picture  order  count)  的编码方法，poc 标识图像的播放顺序。
    //由poc 可以由 frame-num 通过映射关系计算得来，也可以索性由编码器显式地传送。
    //是指解码图像顺序的计数方法（如  8.2.1 节所述）。pic_order_cnt_type 的取值范围是0 到 2（包括0 和2）。
    pic_order_cnt_type = rs.readUE();

    if (pic_order_cnt_type == 0) {
        log2_max_pic_order_cnt_lsb_minus4 = rs.readUE(); //取值范围应该在0-12
        /*表示POC的上限*/
        MaxPicOrderCntLsb = static_cast<uint16_t> (std::pow(2u, log2_max_pic_order_cnt_lsb_minus4 + 4u));
    } else if (this->pic_order_cnt_type == 1) {
        /*等于 1 时,句法元素 delta_pic_order_cnt[0]和 delta_pic_order_cnt[1]不在片头出现,并且它们的值默认为 0;
         * 本句法元素等于 0 时,上述的两个句法元素将在片头出现*/
        delta_pic_order_always_zero_flag = rs.readBit();
        //用于非参考图像的图像顺序号
        offset_for_non_ref_pic = rs.readSE();
        offset_for_top_to_bottom_field = rs.readSE();
        num_ref_frames_in_pic_order_cnt_cycle = rs.readUE();

        for (size_t i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++) {
            offset_for_ref_frame[i] = rs.readSE();
            ExpectedDeltaPerPicOrderCntCycle += offset_for_ref_frame[i];
        }


    }

    //最大允许多少个参考帧 取值范围应该在 0 到 MaxDpbSize
    //MaxDpbSize = Min( 1024 * MaxDPB /  ( PicWidthInMbs * FrameHeightInMbs * 384 ), 16 )
    max_num_ref_frames = rs.readUE();
    //是否允许出现不连续的情况,跳帧
    gaps_in_frame_num_value_allowed_flag = rs.readBit();
    //以红快为单位的宽度
    pic_width_in_mbs_minus1 = rs.readUE();
    //这里要考虑帧编码和场编码的问题，帧编码是红快高度
    pic_height_in_map_units_minus1 = rs.readUE();
    //是否为帧编码
    frame_mbs_only_flag = rs.readBit();


    if (!frame_mbs_only_flag) {
        //采用了场编码的情况下
        //是否采用红快级别帧场自适应的问题 ，是否可以在帧编码和场编码相互切换 //帧场自适应
        // mb_adaptive_frame_field_flag = rs.readBit();
        fprintf(stderr, "场编码\n");
        return -1;

    }

    //B_Skip、B_Direct_16x16 和 B_Direct_8x8 亮度运动矢量 的计算过程使用的方法
    direct_8x8_inference_flag = rs.readBit();
    //图片进行裁剪的偏移量 如果为0那么就不需要裁剪
    frame_cropping_flag = rs.readBit();

    if (frame_cropping_flag) {
        frame_crop_left_offset = rs.readUE(); //frame_crop_left_offset 0 ue(v)
        frame_crop_right_offset = rs.readUE(); //frame_crop_right_offset 0 ue(v)
        frame_crop_top_offset = rs.readUE(); //frame_crop_top_offset 0 ue(v)
        frame_crop_bottom_offset = rs.readUE(); //frame_crop_bottom_offset 0 ue(v)
    }
    vui_parameters_present_flag = rs.readBit();

//    uint8_t fps = 25;
    if (vui_parameters_present_flag) {
        //视频渲染的一些辅助信息
        vui_parameters(rs);

        if (timing_info_present_flag) {
            fps = static_cast<int>(time_scale / num_units_in_tick);
            //if (fixed_frame_rate_flag)
            {
                fps /= 2;
            }

            timeBase = {1, (int) fps};
        }
        //todo
    }


    PicWidthInMbs = pic_width_in_mbs_minus1 + 1;
    PicHeightInMapUnits = pic_height_in_map_units_minus1 + 1;



    //总共有多少宏块
    PicSizeInMapUnits = PicWidthInMbs * PicHeightInMapUnits;
    //如果是场编码要*2
    //FrameHeightInMbs = (2 - frame_mbs_only_flag) * PicHeightInMapUnits;

    //表示UV依附于Y 和Y一起编码，separate_colour_plane_flag=1则表示 UV 与 Y 分开编码。
    if (!separate_colour_plane_flag) {
        ChromaArrayType = chroma_format_idc;
    } else {
        ChromaArrayType = 0;
    }
//    //亮度深度，偏移
//    BitDepthY = 8 + bit_depth_luma_minus8;
//    QpBdOffsetY = 6 * bit_depth_luma_minus8;
//    //色度深度，偏移
//    BitDepthC = 8 + bit_depth_chroma_minus8;
//    QpBdOffsetC = 6 * bit_depth_chroma_minus8;


    /*如果chroma_format_idc的值等于0（单色），则MbWidthC和MbHeightC均为0（单色视频没有色度 阵列）。
    否则，MbWidthC和MbHeightC按下式得到：
    MbWidthC = 16 / SubWidthC
    MbHeightC = 16 / SubHeightC*/
//    if (chroma_format_idc == 0 || separate_colour_plane_flag) {
//        MbWidthC = 0;
//        MbHeightC = 0;
//    } else {
//
//        int index = chroma_format_idc;
//        //separate_colour_plane_flag=0一起编码，=1分开编码
//        if (chroma_format_idc == 3 && separate_colour_plane_flag) {
//            index = 4;
//        }
//        //变量 SubWidthC 和 SubHeightC 在表 6-1 中规定
//        /*在4 : 2 : 0 样点中，两个色度阵列的高度和宽度均为亮度阵列的一半。
//          在4 : 2 : 2 样点中，两个色度阵列的高度等于亮度阵列的高度，宽度为亮度阵列的一半。
//          在4 : 4 : 4 样点中，两个色度阵列的高度和宽度与亮度阵列的相等。*/
//
//
//        //{0, 0, -1, -1},			//单色
//        //{ 1, 0,  2,  2 },		//420
//        //{ 2, 0,  2,  1 },		//422
//        //{ 3, 0,  1,  1 },		//444
//        //{ 3, 1, -1, -1 },		//444   分开编码
//        SubWidthC = chroma_format_idcs[index].SubWidthC;
//        SubHeightC = chroma_format_idcs[index].SubHeightC;
//        /*样点是以宏块为单元进行处理的。每个宏块中的样点阵列的高和宽均为 16 个样点。
//        变量 MbWidthC 和 MbHeightC 分别规定了每个宏块中色度阵列的宽度和高度*/
//        MbWidthC = 16 / SubWidthC;
//        MbHeightC = 16 / SubHeightC;
//
//    }


    //亮度分量的图像宽度
    PicWidthInSamplesL = PicWidthInMbs * 16;
    //亮度分量的图像高度
    PicHeightInSamplesL = PicHeightInMapUnits * 16;


    //色度分量的图像宽度
    //PicWidthInSamplesC = PicWidthInMbs * MbWidthC;
    //色度分量的图像高度
    //PicHeightInSamplesC = PicHeightInMapUnits * MbHeightC;

    //Max_num_reorder_frames表示已解码图片缓冲区中帧数的上限
    //用于在输出前存储帧、互补场对和非成对场
    //max_num_reorder_frames的值应该在0到max_dec_frame_buffering的范围内
    //当max_num_reorder_frames语法元素不存在时，max_num_reorder_frames值的推断如下
    if (max_num_reorder_frames == -1) {
        if ((profile_idc == 44
             || profile_idc == 86
             || profile_idc == 100
             || profile_idc == 110
             || profile_idc == 122
             || profile_idc == 244) && constraint_set3_flag == 1) {
            max_num_reorder_frames = 0;
        } else {//profile_idc is not equal to 44, 86, 100, 110, 122, or 244 or constraint_set3_flag is equal to 0

            //等于MaxDpbFrames
            for (const levelLimits &i: levelTable) {
                if (level_idc == i.MaxDpbMbs) {
                    max_num_reorder_frames = static_cast<int>(std::fmin(
                            i.MaxDpbMbs / (PicWidthInMbs * PicHeightInMapUnits), 16));
                    break;
                }
            }
        }
    }

    return 0;
}

#define Extended_SAR 255


int NALSeqParameterSet::vui_parameters(ReadStream &rs) {
    bool aspect_ratio_info_present_flag = rs.readBit();

    if (aspect_ratio_info_present_flag) {
        /*Aspect_ratio_idc指定luma样本的样本长宽比值。 表E - 1显示了代码的含义。
          当aspect_ratio_idc表示Extended_SAR时，样本长宽比用sar_width和sar_height表示。
          当aspect_ratio_idc语法元素不存在时，可以推断aspect_ratio_idc的值为0
          */
        aspect_ratio_idc = rs.readMultiBit(8);

        if (aspect_ratio_idc == Extended_SAR) {
            //Sar_width和sar_height应相对素数或等于0。
            //当aspect_ratio_idc等于0或sar_width等于0或sar_height等于0时，样本长宽比应被视为本推荐|国际标准未指定
            sar_width = rs.readMultiBit(16);
            sar_height = rs.readMultiBit(16);
        }
    }

    bool overscan_info_present_flag = rs.readBit();
    if (overscan_info_present_flag) {
        //Overscan_appropriate_flag = 1表示经过裁剪的解码图片输出适合超扫描显示。
        //Overscan_appropriate_flag = 0表示经过裁剪的解码图片输出包含了从图片裁剪矩形到边缘的整个区域的重要信息，因此经过裁剪的解码图片输出不应该使用超扫描显示。
        //相反，它们应该使用显示区域和剪切矩形之间的精确匹配或使用下扫描来显示。
        overscan_appropriate_flag = rs.readBit();
    }

    bool video_signal_type_present_flag = rs.readBit();

    if (video_signal_type_present_flag) {
        //video_format表示表E-2中指定的图片的表示形式，然后根据本建议|国际标准进行编码。
        //当video_format语法元素不存在时，可以推断出video_format的值等于5。

        /*
        表 E-2
        0		Component
        1		PAL
        2		NTSC
        3		SECAM
        4		MAC
        5		Unspecified video
        */
        video_format = rs.readMultiBit(3);

        //video_full_range_flag 表示范围
        /*
        If video_full_range_flag is equal to 0,
            Y = Round( 219 * E’Y + 16 )
            Cb = Round( 224 * E’PB + 128 )
            Cr = Round( 224 * E’PR + 128 )
        -  Otherwise (video_full_range_flag is equal to 1),
            Y = Round( 255 * E’Y )
            Cb = Round( 255 * E’PB + 128 )
            Cr = Round( 255 * E’PR + 128 )
        */
        video_full_range_flag = rs.readBit();
        bool colour_description_present_flag = rs.readBit();
        if (colour_description_present_flag) {
            colour_primaries = rs.readMultiBit(8);
            transfer_characteristics = rs.readMultiBit(8);
            matrix_coefficients = rs.readMultiBit(8);
        }
    }

    bool chroma_loc_info_present_flag = rs.readBit();

    if (chroma_loc_info_present_flag) {
        chroma_sample_loc_type_top_field = rs.readUE();
        chroma_sample_loc_type_bottom_field = rs.readUE();
    }


    timing_info_present_flag = rs.readBit();

    if (timing_info_present_flag) {
        //一个增量所用的time units
        num_units_in_tick = rs.readMultiBit(32);
        //即将1秒分成多少份
        time_scale = rs.readMultiBit(32);

        //fixed_frame_rate_flag为1时，两个连续图像的HDR输出时间频率为单位，获取的fps是实际的2倍
        fixed_frame_rate_flag = rs.readBit();
    }

    bool nal_hrd_parameters_present_flag = rs.readBit();
    if (nal_hrd_parameters_present_flag) {
        //hrd_parameters( )
        fprintf(stderr, "non-supported hrd_parameters\n");
        //return -1;
    }

    bool vcl_hrd_parameters_present_flag = rs.readBit();
    if (vcl_hrd_parameters_present_flag) {
        //hrd_parameters()
        fprintf(stderr, "non-supported hrd_parameters\n");
        //return -1;
    }

    if (nal_hrd_parameters_present_flag || vcl_hrd_parameters_present_flag) {
        low_delay_hrd_flag = rs.readBit();
    }

    bool pic_struct_present_flag = rs.readBit();

    bool bitstream_restriction_flag = rs.readBit();
    if (bitstream_restriction_flag) {
        bool motion_vectors_over_pic_boundaries_flag = rs.readBit();
        uint32_t max_bytes_per_pic_denom = rs.readUE();
        uint32_t max_bits_per_mb_denom = rs.readUE();
        uint32_t log2_max_mv_length_horizontal = rs.readUE();
        uint32_t log2_max_mv_length_vertical = rs.readUE();
        max_num_reorder_frames = static_cast<int>(rs.readUE());
        max_dec_frame_buffering = static_cast<int>(rs.readUE());
    }
    return 0;
}

#undef Extended_SAR