

#include "NALPicture.h"

#include <cstdio>


/*
 * 解码POC需要用到上一个参考帧，和上一个顺序解码的帧，那就肯定要缓存帧，以做解码，
 * 所以需要标记参考帧，放进参考帧列表里面，并且也要有参考帧标记过程，把一些不用于参考的给剔除，
 * 你也得要有参考标记过程，标记哪个是参考帧，所以之前那个是必须的，
 * 所以解码POC需要有参考帧标记流程，因为用到了上一个参考帧
 * */

int NALPicture::decoding_process_for_picture_order_count() {
    int ret = 0;
    if (sliceHeader.sps.pic_order_cnt_type == 0) {
        ret = decoding_process_for_picture_order_count_type_0();
        if (ret < 0) {
            fprintf(stderr, "解码POC失败，pic_order_cnt_type = 0\n");
            return ret;
        }
    } else if (sliceHeader.sps.pic_order_cnt_type == 1) {
        ret = decoding_process_for_picture_order_count_type_1();
        if (ret < 0) {
            fprintf(stderr, "解码POC失败，pic_order_cnt_type = 1\n");
            return ret;
        }
    } else if (sliceHeader.sps.pic_order_cnt_type == 2) {
        ret = decoding_process_for_picture_order_count_type_2();
        if (ret < 0) {
            fprintf(stderr, "解码POC失败，pic_order_cnt_type = 2\n");
            return ret;
        }
    } else {
        fprintf(stderr, "解码POC失败，pic_order_cnt_type = %d\n", sliceHeader.sps.pic_order_cnt_type);
        return -1;
    }
    pictureOrderCount = std::min(TopFieldOrderCnt, BottomFieldOrderCnt);
    //printf("POC = %d\n", pictureOrderCount);
    /*计算出POC*/
    return ret;
}

int NALPicture::decoding_process_for_picture_order_count_type_0() {
    //前一个参考图像的 PicOrderCntMsb
    uint32_t prevPicOrderCntMsb;
    uint32_t prevPicOrderCntLsb;
    if (sliceHeader.nalu.IdrPicFlag) {
        prevPicOrderCntMsb = 0;
        prevPicOrderCntLsb = 0;
    } else {
        if (referencePicture == nullptr) {
            fprintf(stderr, "上一个参考帧为null\n");
            return -1;
        }
        //如果前面的参考图片在解码顺序中包含一个memory_management_control_operation等于5
        if (referencePicture->memory_management_control_operation_5_flag) {
            //如果在解码顺序上前一个参考图像不是底场，则prevPicOrderCntMsb设置为0,prevPicOrderCntLsb设置为解码顺序上前一个参考图像的TopFieldOrderCnt值。
            prevPicOrderCntMsb = 0;
            prevPicOrderCntLsb = referencePicture->TopFieldOrderCnt;
        } else {
            prevPicOrderCntMsb = referencePicture->PicOrderCntMsb;
            prevPicOrderCntLsb = referencePicture->sliceHeader.pic_order_cnt_lsb;
        }
    }

    if ((sliceHeader.pic_order_cnt_lsb < prevPicOrderCntLsb)
        && ((prevPicOrderCntLsb - sliceHeader.pic_order_cnt_lsb) >= (sliceHeader.sps.MaxPicOrderCntLsb / 2))
            ) {
        /*当前图像的pic_order_cnt_lsb比上一针小，且未发生乱序*/
        PicOrderCntMsb = prevPicOrderCntMsb + sliceHeader.sps.MaxPicOrderCntLsb;
    } else if ((sliceHeader.pic_order_cnt_lsb > prevPicOrderCntLsb)
               && ((sliceHeader.pic_order_cnt_lsb - prevPicOrderCntLsb) > (sliceHeader.sps.MaxPicOrderCntLsb / 2))) {
        /*当前图像的pic_order_cnt_lsb比上一针大，且发生乱序*/
        PicOrderCntMsb = prevPicOrderCntMsb - sliceHeader.sps.MaxPicOrderCntLsb;
    } else {
        PicOrderCntMsb = prevPicOrderCntMsb;
    }

    // 当前图像为非底场时
    TopFieldOrderCnt = PicOrderCntMsb + sliceHeader.pic_order_cnt_lsb;
    // 当前图像为非顶场时
    BottomFieldOrderCnt = TopFieldOrderCnt + sliceHeader.delta_pic_order_cnt_bottom;

    return 0;
}

int NALPicture::decoding_process_for_picture_order_count_type_1() {

    if (previousPicture == nullptr) {
        fprintf(stderr, "上一顺序解码的图像不存在\n");
        return -1;
    }
    const uint16_t prevFrameNum = previousPicture->sliceHeader.frame_num;
    int prevFrameNumOffset = 0;

    if (!sliceHeader.nalu.IdrPicFlag) {
        if (previousPicture->memory_management_control_operation_5_flag) {
            //If the previous picture in decoding order included a memory_management_control_operation equal to 5
            prevFrameNumOffset = 0;
        } else {
            prevFrameNumOffset = previousPicture->FrameNumOffset;
        }
    }


    if (sliceHeader.nalu.IdrPicFlag) {
        FrameNumOffset = 0;
    } else if (prevFrameNum > sliceHeader.frame_num) {
        FrameNumOffset = prevFrameNumOffset + sliceHeader.sps.MaxFrameNum;
    } else {
        FrameNumOffset = prevFrameNumOffset;
    }

    int absFrameNum = 0;
    if (sliceHeader.sps.num_ref_frames_in_pic_order_cnt_cycle != 0) {
        absFrameNum = FrameNumOffset + sliceHeader.frame_num;
    }

    if (sliceHeader.nalu.nal_ref_idc == 0 && absFrameNum > 0) {
        absFrameNum = absFrameNum - 1;
    }

    int picOrderCntCycleCnt;
    int frameNumInPicOrderCntCycle;
    if (absFrameNum > 0) {
        picOrderCntCycleCnt = (absFrameNum - 1) / sliceHeader.sps.num_ref_frames_in_pic_order_cnt_cycle;
        frameNumInPicOrderCntCycle = (absFrameNum - 1) % sliceHeader.sps.num_ref_frames_in_pic_order_cnt_cycle;
    }

    int expectedPicOrderCnt = 0;
    if (absFrameNum > 0) {
        expectedPicOrderCnt = picOrderCntCycleCnt * sliceHeader.sps.ExpectedDeltaPerPicOrderCntCycle;
        for (int i = 0; i <= frameNumInPicOrderCntCycle; i++)
            expectedPicOrderCnt = expectedPicOrderCnt + sliceHeader.sps.offset_for_ref_frame[i];
    }

    if (sliceHeader.nalu.nal_ref_idc == 0) {
        expectedPicOrderCnt = expectedPicOrderCnt + sliceHeader.sps.offset_for_non_ref_pic;
    }
    TopFieldOrderCnt = expectedPicOrderCnt + sliceHeader.delta_pic_order_cnt[0];
    BottomFieldOrderCnt =
            TopFieldOrderCnt + sliceHeader.sps.offset_for_top_to_bottom_field + sliceHeader.delta_pic_order_cnt[1];
    return 0;
}

int NALPicture::decoding_process_for_picture_order_count_type_2() {

    if (previousPicture == nullptr) {
        fprintf(stderr, "上一顺序解码的图像不存在\n");
        return -1;
    }

    const uint16_t prevFrameNum = previousPicture->sliceHeader.frame_num;

    int prevFrameNumOffset;

    if (!sliceHeader.nalu.IdrPicFlag) {
        if (previousPicture->memory_management_control_operation_5_flag) {
            prevFrameNumOffset = 0;
        } else {
            prevFrameNumOffset = previousPicture->FrameNumOffset;
        }
    }

    if (sliceHeader.nalu.IdrPicFlag)
        FrameNumOffset = 0;
    else if (prevFrameNum > sliceHeader.frame_num)
        FrameNumOffset = prevFrameNumOffset + sliceHeader.sps.MaxFrameNum;
    else
        FrameNumOffset = prevFrameNumOffset;


    int tempPicOrderCnt;
    if (sliceHeader.nalu.IdrPicFlag) {
        tempPicOrderCnt = 0;
    } else if (sliceHeader.nalu.nal_ref_idc == 0) {
        //当前图像为非参考图像
        tempPicOrderCnt = 2 * (FrameNumOffset + sliceHeader.frame_num) - 1;
    } else {
        tempPicOrderCnt = 2 * (FrameNumOffset + sliceHeader.frame_num);
    }


    TopFieldOrderCnt = tempPicOrderCnt;
    BottomFieldOrderCnt = tempPicOrderCnt;


    return 0;
}

void NALPicture::reset() {
    pictureFinishFlag = false;
    useFlag = false;
    size = 0;
    dts = 0;
    pts = 0;
    pcr = 0;
    duration = 0;
    pictureOrderCount = 0;
    previousPicture = nullptr;
    referencePicture = nullptr;
    reference_marked_type = UNUSED_FOR_REFERENCE;
    MaxLongTermFrameIdx = -1;
    LongTermFrameIdx = -1;
    FrameNum = 0;
    FrameNumWrap = -1;
    PicNum = -1;
    LongTermPicNum = -1;

    memory_management_control_operation_5_flag = false;
    memory_management_control_operation_6_flag = false;


    FrameNumOffset = 0;
    PicOrderCntMsb = 0;
    /*while (!data.empty()) {
        uint8_t *buf = data.back().data;
        delete[] buf;
        data.pop_back();
    }*/
    data.clear();
}

NALPicture::~NALPicture() {
    /*while (!data.empty()) {
        uint8_t *buf = data.back().data;
        delete[] buf;
        data.pop_back();
    }*/
}
