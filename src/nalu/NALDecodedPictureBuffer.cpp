
#include "NALDecodedPictureBuffer.h"

#include <cstdio>

#include "NALHeader.h"
#include "NALSliceHeader.h"

NALDecodedPictureBuffer::NALDecodedPictureBuffer() {
    for (NALPicture *&pic: dpb) {
        pic = new NALPicture;
    }
}

void NALDecodedPictureBuffer::reset() {
    for (NALPicture *&pic: dpb) {
        if (pic) pic->reset();
    }
}

int NALDecodedPictureBuffer::decoding_finish(NALPicture *picture, NALPicture *&unoccupiedPicture) {
    int ret;
    /*This process is invoked for decoded pictures when nal_ref_idc is not equal to 0.*/
    if (picture->sliceHeader.nalu.nal_ref_idc != 0) {
        //标记当前解码完成的帧是什么类型
        /*nal_ref_idc不等于0的解码图片称为参考图片，被标记为“用于短期参考”或“用于长期参考”。*/
        ret = decoded_reference_picture_marking_process(picture);
        if (ret < 0) {
            fprintf(stderr, "图像标记失败\n");
            return ret;
        }
    }


    /*找到一个空闲的picture*/
    unoccupiedPicture = getUnoccupiedPicture(picture);
    if (unoccupiedPicture == nullptr) {
        fprintf(stderr, "没有在dpb找到空闲的picture\n");
        return -1;
    }
    /*重置*/
    unoccupiedPicture->reset();

    unoccupiedPicture->previousPicture = picture;
    if (picture->reference_marked_type != UNUSED_FOR_REFERENCE) {
        unoccupiedPicture->referencePicture = picture;
    } else {
        unoccupiedPicture->referencePicture = picture->referencePicture;
    }
    return 0;
}

int NALDecodedPictureBuffer::decoded_reference_picture_marking_process(NALPicture *picture) {

    const NALSliceHeader &sliceHeader = picture->sliceHeader;


    if (sliceHeader.nalu.IdrPicFlag) {
        /*All reference pictures are marked as "unused for reference"*/
        for (NALPicture *pic: dpb) {
            pic->reference_marked_type = UNUSED_FOR_REFERENCE;
        }

        if (sliceHeader.long_term_reference_flag) {
            picture->reference_marked_type = LONG_TERM_REFERENCE;
            picture->MaxLongTermFrameIdx = 0;
            picture->LongTermFrameIdx = 0;
        } else {
            picture->reference_marked_type = SHORT_TERM_REFERENCE;
            picture->MaxLongTermFrameIdx = -1;
        }
    } else {
        if (sliceHeader.adaptive_ref_pic_marking_mode_flag) {
            adaptive_memory_control_decoded_reference_picture_marking_process(picture);
        } else {
            //8.2.5.3 Sliding window decoded reference picture marking process 滑动窗口解码参考图像的标识过程
            sliding_window_decoded_reference_picture_marking_process(picture);
        }
    }

    /*当前图片不是IDR图片，且memory_management_control_operation未将其标记为“用于长期参考”时，则将其标记为“用于短期参考”。*/
    if (!sliceHeader.nalu.IdrPicFlag && !picture->memory_management_control_operation_6_flag) {
        picture->reference_marked_type = SHORT_TERM_REFERENCE;
        //设置为没有长期索引
        picture->MaxLongTermFrameIdx = -1;
        picture->LongTermFrameIdx = -1;
    }
    return 0;
}

int NALDecodedPictureBuffer::sliding_window_decoded_reference_picture_marking_process(const NALPicture *picture) {

    int numShortTerm = 0;
    int numLongTerm = 0;

    for (NALPicture *pic: dpb) {
        if (pic->reference_marked_type == SHORT_TERM_REFERENCE) {
            ++numShortTerm;
        }
        if (pic->reference_marked_type == LONG_TERM_REFERENCE) {
            ++numLongTerm;
        }
    }
    if (numShortTerm + numLongTerm == std::max<uint8_t>(picture->sliceHeader.sps.max_num_ref_frames, 1u) &&
        numShortTerm > 0
            ) {
        /*有着最小 FrameNumWrap 值的短期参考帧标记为“不用于参考”*/

        int idx = -1;
        int FrameNumWrap = -1;

        for (int i = 0; i < 16; ++i) {
            if (dpb[i]->reference_marked_type == SHORT_TERM_REFERENCE) {

                if (idx == -1) {
                    idx = i;
                    FrameNumWrap = dpb[i]->FrameNumWrap;
                    continue;
                }

                if (dpb[i]->FrameNumWrap < FrameNumWrap) {
                    FrameNumWrap = dpb[i]->FrameNumWrap;
                    idx = i;
                }
            }
        }
        if (idx != -1) {
            dpb[idx]->reference_marked_type = UNUSED_FOR_REFERENCE;
        }
    }

    return 0;
}

int NALDecodedPictureBuffer::adaptive_memory_control_decoded_reference_picture_marking_process(NALPicture *picture) {
    const NALSliceHeader &sliceHeader = picture->sliceHeader;
    for (size_t i = 0; i < sliceHeader.dec_ref_pic_markings_size; i++) {

        if (sliceHeader.dec_ref_pic_markings[i].memory_management_control_operation == 1) {
            /*将短期参考图片标记为“不用于参考”的过程*/
            /*picNumX 用来指定一个标记为“用于短期参考”且未被标记为“不存在”的帧*/
            int picNumX =
                    sliceHeader.CurrPicNum - (sliceHeader.dec_ref_pic_markings[i].difference_of_pic_nums_minus1 + 1);
            for (NALPicture *pic: dpb) {
                /*picNumX指定的短期参考系或短期互补参考字段对，其字段都标记为“未使用参考”*/
                if (pic->reference_marked_type == SHORT_TERM_REFERENCE && pic->PicNum == picNumX) {
                    pic->reference_marked_type = UNUSED_FOR_REFERENCE;
                }
            }

        } else if (sliceHeader.dec_ref_pic_markings[i].memory_management_control_operation == 2) {
            /*将长期参考图片标记为“不用于参考”的过程*/
            for (NALPicture *pic: dpb) {
                if (pic->reference_marked_type == LONG_TERM_REFERENCE &&
                    pic->LongTermPicNum == sliceHeader.dec_ref_pic_markings[i].long_term_pic_num) {
                    pic->reference_marked_type = UNUSED_FOR_REFERENCE;
                }
            }
        } else if (sliceHeader.dec_ref_pic_markings[i].memory_management_control_operation == 3) {
            /*分配LongTermFrameIdx到短期参考图片的过程*/
            int picNumX =
                    sliceHeader.CurrPicNum - (sliceHeader.dec_ref_pic_markings[i].difference_of_pic_nums_minus1 + 1);

            /*当LongTermFrameIdx等于long_term_frame_idx已经被分配给一个长期参考帧,该帧被标记为“未使用的参考”*/
            for (NALPicture *pic: dpb) {
                if (pic->reference_marked_type == LONG_TERM_REFERENCE
                    && pic->LongTermFrameIdx == sliceHeader.dec_ref_pic_markings[i].long_term_frame_idx) {
                    pic->reference_marked_type = UNUSED_FOR_REFERENCE;
                }
            }
            for (NALPicture *pic: dpb) {
                if (pic->reference_marked_type == SHORT_TERM_REFERENCE && pic->PicNum == picNumX) {
                    pic->reference_marked_type = LONG_TERM_REFERENCE;
                    picture->LongTermFrameIdx = sliceHeader.dec_ref_pic_markings[i].long_term_frame_idx;
                }
            }

        } else if (sliceHeader.dec_ref_pic_markings[i].memory_management_control_operation == 4) {
            /*基于MaxLongTermFrameIdx的标记过程*/


            for (NALPicture *pic: dpb) {
                if ((pic->LongTermFrameIdx > sliceHeader.dec_ref_pic_markings[i].max_long_term_frame_idx_plus1 - 1) &&
                    pic->reference_marked_type == LONG_TERM_REFERENCE) {
                    pic->reference_marked_type = UNUSED_FOR_REFERENCE;
                }
            }

            if (sliceHeader.dec_ref_pic_markings[i].max_long_term_frame_idx_plus1 == 0) {
                picture->MaxLongTermFrameIdx = -1;//非长期帧索引
            } else {
                picture->MaxLongTermFrameIdx = sliceHeader.dec_ref_pic_markings[i].max_long_term_frame_idx_plus1 - 1;
            }
        } else if (sliceHeader.dec_ref_pic_markings[i].memory_management_control_operation == 5) {
            /*所有参考图像标记为“不用于参考”*/
            for (NALPicture *pic: dpb) {
                pic->reference_marked_type = UNUSED_FOR_REFERENCE;
            }

            picture->MaxLongTermFrameIdx = -1;
            picture->memory_management_control_operation_5_flag = true;

        } else if (sliceHeader.dec_ref_pic_markings[i].memory_management_control_operation == 6) {
            /*为当前图片分配长期帧索引的过程*/
            for (NALPicture *pic: dpb) {
                if (pic->LongTermFrameIdx == sliceHeader.dec_ref_pic_markings[i].long_term_frame_idx &&
                    pic->reference_marked_type == LONG_TERM_REFERENCE) {
                    pic->reference_marked_type = UNUSED_FOR_REFERENCE;
                }
            }
            picture->reference_marked_type = LONG_TERM_REFERENCE;
            picture->LongTermFrameIdx = sliceHeader.dec_ref_pic_markings[i].long_term_frame_idx;
            picture->memory_management_control_operation_6_flag = true;
        }

    }
    return 0;
}

int NALDecodedPictureBuffer::decoding_process_for_reference_picture_lists_construction(NALPicture *picture) {
    /*图像序号的计算*/
    decoding_process_for_picture_numbers(picture);


    /*参考帧列表初始化*/
    /*这里不解码，就不用去初始化了*/
    /*Initialisation_process_for_reference_picture_lists*/

    /*参考帧列表重排序*/
    /*这里不解码，就不用去排序了*/
    /*Modification_process_for_reference_picture_lists*/
    return 0;
}

int NALDecodedPictureBuffer::decoding_process_for_picture_numbers(NALPicture *picture) {
    const NALSliceHeader &sliceHeader = picture->sliceHeader;
    /*
     * 对于每个短期参考图片，变量FrameNum和FrameNumWrap被分配如下。
     * 首先，FrameNum被设置为等于语法元素frame_num，
     * 该语法元素已在相应的短期参考图片的片标头中解码。
     * 然后，变量FrameNumWrap被派生为:
     * */
    picture->FrameNum = sliceHeader.frame_num;
    /*其中使用的frame_num的值是当前图片切片头中的frame_num*/
    for (NALPicture *pic: dpb) {
        //对每一个短期参考图像
        if (pic->reference_marked_type == SHORT_TERM_REFERENCE) {
            if (pic->FrameNum > sliceHeader.frame_num) {
                pic->FrameNumWrap = pic->FrameNum - pic->sliceHeader.sps.MaxFrameNum;
            } else {
                pic->FrameNumWrap = pic->FrameNum;
            }
            pic->PicNum = pic->FrameNumWrap;
        }

        if (pic->reference_marked_type == LONG_TERM_REFERENCE) {
            pic->LongTermPicNum = pic->LongTermFrameIdx;
        }
    }


    return 0;
}

NALPicture *NALDecodedPictureBuffer::getUnoccupiedPicture(const NALPicture *picture) {
    NALPicture *ret = nullptr;
    for (NALPicture *pic: dpb) {
        if (pic != picture &&
            !pic->useFlag &&
            pic->reference_marked_type == UNUSED_FOR_REFERENCE
                ) {

            ret = pic;
            break;
        }
    }
    return ret;
}

NALDecodedPictureBuffer::~NALDecodedPictureBuffer() {
    for (NALPicture *&pic: dpb) {
        delete pic;
        pic = nullptr;
    }
}







