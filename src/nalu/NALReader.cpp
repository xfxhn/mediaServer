#include "NALReader.h"
#include "readStream.h"




//uint32_t decode_first_mb_in_slice(uint8_t *data, uint32_t size) {
//    /*先去掉防竞争字节*/
////    NALHeader::ebsp_to_rbsp(data, size);
//    ReadStream rs(data + 1, size);
//    return rs.readUE();
//
//}
//struct Picture {
//    uint8_t *data{nullptr};
//    uint32_t size{0};
//    bool useFlag{false};
//    bool maker{false};
//};
//
//int NALReader::getVideoFrame(Picture &picture, bool &flag) {
//    int ret;
//
//    if (picture.useFlag) {
//        picture.useFlag = false;
//        uint8_t *data = picture.data;
//        picture = unoccupiedPicture;
//        unoccupiedPicture.data = data;
//    }
//
//
//    uint8_t *data = nullptr;
//    uint32_t size = 0;
//    int startCodeLength = 0;
//    while (flag) {
//        ret = readNalUint(data, size, startCodeLength, flag);
//        if (ret < 0) {
//            fprintf(stderr, "读取nal uint发生错误\n");
//            return ret;
//        }
//
//        /*读取nalu header*/
//        nalUnitHeader.nal_unit(data[0]);
//        if (nalUnitHeader.nal_unit_type == H264_NAL_SPS) {
//            if (picture.useFlag) {
//                picture.maker = true;
//                unoccupiedPicture.size = size;
//                unoccupiedPicture.maker = false;
//                unoccupiedPicture.useFlag = true;
//                memcpy(unoccupiedPicture.data, data, size);
//                return 0;
//            }
//
//
//            /*直接把当前这个nalu返回出去*/
//            picture.size = size;
//            picture.useFlag = false;
//            memcpy(picture.data, data, size);
//            return 0;
//        } else if (nalUnitHeader.nal_unit_type == H264_NAL_PPS) {
//            if (picture.useFlag) {
//                picture.maker = true;
//                unoccupiedPicture.size = size;
//                unoccupiedPicture.maker = false;
//                unoccupiedPicture.useFlag = true;
//                memcpy(unoccupiedPicture.data, data, size);
//                return 0;
//            }
//
//
//            picture.size = size;
//            picture.useFlag = false;
//            memcpy(picture.data, data, size);
//
//            return 0;
//        } else if (nalUnitHeader.nal_unit_type == H264_NAL_SLICE || nalUnitHeader.nal_unit_type == H264_NAL_IDR_SLICE) {
//            /*当前nalu的 slice number*/
//            uint32_t first_mb_in_slice = decode_first_mb_in_slice(data, size);
//
//            /*useFlag=true,表示有数据*/
//            if (first_mb_in_slice == 0 && picture.useFlag) {
//                picture.maker = true;
//                /*设置这个空闲的帧的数据*/
//                unoccupiedPicture.size = size;
//                unoccupiedPicture.useFlag = true;
//                memcpy(unoccupiedPicture.data, data, size);
//                return 0;
//            } else if (picture.useFlag) {
//                picture.maker = false;
//
//                unoccupiedPicture.size = size;
//                unoccupiedPicture.useFlag = true;
//                memcpy(unoccupiedPicture.data, data, size);
//                return 0;
//            } else {
//                /*保存当前nalu的数据*/
//                picture.useFlag = true;
//                picture.size = size;
//                memcpy(picture.data, data, size);
//            }
//        }
//
//    }
//    return 0;
//}

static uint64_t av_rescale_q(uint64_t a, const AVRational &bq, const AVRational &cq) {
    //(1 / 25) / (1 / 1000);
    int64_t b = bq.num * cq.den;
    int64_t c = cq.num * bq.den;
    return a * b / c;  //25 * (1000 / 25)  把1000分成25份，然后当前占1000的多少
}

int NALReader::init(const char *filename) {
    fs.open(filename, std::ios::out | std::ios::binary);

    if (!fs.is_open()) {
        fprintf(stderr, "open %s failed\n", filename);
        return -1;
    }
    bufferStart = new uint8_t[MAX_BUFFER_SIZE];
    fs.read(reinterpret_cast<char *>(bufferStart), MAX_BUFFER_SIZE);
    blockBufferSize = fs.gcount();

    bufferPosition = bufferStart;
    bufferEnd = bufferStart + blockBufferSize - 1;

    return 0;
}

void NALReader::reset() {
    fs.seekg(0);
    fs.read(reinterpret_cast<char *>(bufferStart), MAX_BUFFER_SIZE);
    blockBufferSize = fs.gcount();
    bufferPosition = bufferStart;
    bufferEnd = bufferStart + blockBufferSize - 1;

    delete[] spsData;
    spsData = nullptr;
    delete[] ppsData;
    ppsData = nullptr;
    videoDecodeFrameNumber = 0;
    videoDecodeIdrFrameNumber = 0;
    unoccupiedPicture = nullptr;
    gop.reset();
    finishFlag = false;
}


int NALReader::readNalUint(uint8_t *&data, uint32_t &size, int &startCodeLength, bool &isStopLoop) {

    while (true) {
        uint8_t *pos1 = nullptr;
        uint8_t *pos2 = nullptr;
        int startCodeLen1 = 0;
        int startCodeLen2 = 0;

        const int type = getNextStartCode(bufferPosition, bufferEnd,
                                          pos1, pos2, startCodeLen1, startCodeLen2);
        startCodeLength = startCodeLen1;
        /*还剩多少字节未读取*/
        uint32_t residual = (bufferEnd - pos1 + 1);
        /*已经读取了多少个字节*/
        uint32_t readSize = blockBufferSize - residual;
        if (type == 1) { //表示找到了开头的startCode,没找到后面的
            memcpy(bufferStart, pos1, residual);
            //每次读File::MAX_BUFFER_SIZE个，这里读取的NALU必须要包含一整个slice,字节对齐
            //size_t bufferSize = fread(bufferStart + residual, 1, readSize, file);
            fs.read(reinterpret_cast<char *>(bufferStart + residual), readSize);
            uint32_t bufferSize = fs.gcount();

            if (bufferSize == 0) {
                //表示读完数据了
                size = residual - startCodeLen1;
                data = bufferStart + startCodeLen1;
                isStopLoop = false;
                break;
            }
            blockBufferSize = residual + bufferSize;
            bufferPosition = bufferStart;
            bufferEnd = bufferStart + blockBufferSize - 1;
        } else if (type == 2) {//都找到了
            data = pos1 + startCodeLen1;
            size = pos2 - data;
            bufferPosition = pos2;// data + size;
            break;
        } else {
            //错误
            fprintf(stderr, "没有找到开头startCode，也没有找到后面的startCode\n");
            isStopLoop = false;
            return -1;
        }
    }
    return 0;
}

int NALReader::getNextStartCode(uint8_t *bufPtr, const uint8_t *end,
                                uint8_t *&pos1, uint8_t *&pos2, int &startCodeLen1, int &startCodeLen2) {
    uint8_t *pos = bufPtr;
    int type = 0;
    //查找开头的startCode
    while (pos < end) {
        if (pos[0] == 0) {
            if (pos[1] == 0) {
                if (pos[2] == 0) {
                    if (pos[3] == 1) { //0 0 0 1
                        startCodeLen1 = 4;
                        pos1 = pos;
                        pos += startCodeLen1;
                        type = 1;
                        break;

                    }
                } else if (pos[2] == 1) {  //0 0 1
                    startCodeLen1 = 3;
                    pos1 = pos;
                    pos += startCodeLen1;
                    type = 1;
                    break;
                }
            }
        }
        pos++;
    }
    //查找结尾的startCode
    while (pos < end) {
        if (pos[0] == 0) {
            if (pos[1] == 0) {
                if (pos[2] == 0) {
                    if (pos[3] == 1) { //0 0 0 1
                        pos2 = pos;
                        startCodeLen2 = 4;
                        type = 2;
                        break;
                    }
                } else if (pos[2] == 1) {  //0 0 1
                    pos2 = pos;
                    startCodeLen2 = 3;
                    type = 2;
                    break;
                }

            }
        }
        pos++;
    }
    return type;
}


int NALReader::getSpsAndPps(uint32_t &spsSize, uint32_t &ppsSize) {
    int ret = 0;


    uint8_t *data = nullptr;
    uint32_t size = 0;
    int startCodeLength = 0;
    bool flag = true;
    while (flag) {
        ret = readNalUint(data, size, startCodeLength, flag);
        if (ret < 0) {
            fprintf(stderr, "读取nal uint发生错误\n");
            return ret;
        }

        nalUnitHeader.nal_unit(data[0]);
        if (nalUnitHeader.nal_unit_type == H264_NAL_SPS) {
            delete[] spsData;
            spsData = nullptr;
            spsData = new uint8_t[size];
            memcpy(spsData, data, size);
            spsSize = size;

            /*去除防竞争字节*/
            NALHeader::ebsp_to_rbsp(data, size);
            ReadStream rs(data, size);


            sps.seq_parameter_set_data(rs);
            spsList[sps.seq_parameter_set_id] = sps;


        } else if (nalUnitHeader.nal_unit_type == H264_NAL_PPS) {
            delete[] ppsData;
            ppsData = nullptr;
            ppsData = new uint8_t[size];
            memcpy(ppsData, data, size);
            ppsSize = size;

            NALHeader::ebsp_to_rbsp(data, size);
            ReadStream rs(data, size);


            pps.pic_parameter_set_rbsp(rs, spsList);
            ppsList[pps.pic_parameter_set_id] = pps;


            if (spsData == nullptr) {
                fprintf(stderr, "没找到sps\n");
                return -1;
            }

            /*读取完SPS和PPS了,退出*/
            return 0;
        }
    }
    return 0;
}

int NALReader::getVideoFrame(NALPicture *&picture, bool &flag) {


    int ret;
    if (finishFlag) {
        flag = false;
        /*计算pts和dts*/
        computedTimestamp(unoccupiedPicture);
        picture = unoccupiedPicture;
        return 0;
    }

    if (picture->useFlag) {
        /*把当前这个picture标记为不在使用，可以释放*/
        picture->useFlag = false;
        picture = unoccupiedPicture;
    }

    uint8_t *data = nullptr;
    uint32_t size = 0;
    int startCodeLength = 0;

    while (flag) {
        /*每次读取一个nalu*/
        ret = readNalUint(data, size, startCodeLength, flag);
        if (ret < 0) {
            fprintf(stderr, "读取nal uint发生错误\n");
            return ret;
        }

        /*读取nalu header*/
        nalUnitHeader.nal_unit(data[0]);

        if (nalUnitHeader.nal_unit_type == H264_NAL_SPS) {
            const uint32_t tempSize = size;
            uint8_t *buf = new uint8_t[tempSize]; // NOLINT(modernize-use-auto)
            memcpy(buf, data, tempSize);

            /*去除防竞争字节*/
            NALHeader::ebsp_to_rbsp(data, size);
            ReadStream rs(data, size);

            sps.seq_parameter_set_data(rs);
            spsList[sps.seq_parameter_set_id] = sps;

            if (picture->useFlag) {
                /*计算pts和dts*/
                computedTimestamp(picture);
                /*
                   * 参考图片标记过程
                   * 当前图片的所有SLICE都被解码。
                   * 参考8.2.5.1第一条规则
                   * */
                /*这里标记了这个使用的帧是长期参考还是短期参考，并且给出一个空闲的帧*/
                ret = gop.decoding_finish(picture, unoccupiedPicture);
                if (ret < 0) {
                    fprintf(stderr, "图像解析失败\n");
                    return ret;
                }

                unoccupiedPicture->size += tempSize;
                unoccupiedPicture->data.push_back({tempSize, buf, 0});
                return 0;
            }

            picture->size += tempSize;
            picture->data.push_back({tempSize, buf, 1});

        } else if (nalUnitHeader.nal_unit_type == H264_NAL_PPS) {
            const uint32_t tempSize = size;
            uint8_t *buf = new uint8_t[tempSize]; // NOLINT(modernize-use-auto)
            memcpy(buf, data, tempSize);


            NALHeader::ebsp_to_rbsp(data, size);
            ReadStream rs(data, size);


            pps.pic_parameter_set_rbsp(rs, spsList);
            ppsList[pps.pic_parameter_set_id] = pps;
            if (picture->useFlag) {
                computedTimestamp(picture);
                /*
                    * 参考图片标记过程
                    * 当前图片的所有SLICE都被解码。
                    * 参考8.2.5.1第一条规则
                    * */
                /*这里标记了这个使用的帧是长期参考还是短期参考，并且给出一个空闲的帧*/
                ret = gop.decoding_finish(picture, unoccupiedPicture);
                if (ret < 0) {
                    fprintf(stderr, "图像解析失败\n");
                    return ret;
                }

                unoccupiedPicture->size += tempSize;
                unoccupiedPicture->data.push_back({tempSize, buf, 0});
                return 0;
            }

            picture->size += tempSize;
            picture->data.push_back({tempSize, buf, 2});
        } else if (nalUnitHeader.nal_unit_type == H264_NAL_SLICE || nalUnitHeader.nal_unit_type == H264_NAL_IDR_SLICE) {
            const uint32_t tempSize = size;
            uint8_t *buf = new uint8_t[tempSize]; // NOLINT(modernize-use-auto)
            memcpy(buf, data, tempSize);

            NALHeader::ebsp_to_rbsp(data, size);
            ReadStream rs(data, size);

            //    NALSliceHeader sliceHeader;
            sliceHeader.slice_header(rs, nalUnitHeader, spsList, ppsList);

            /*先处理上一帧，如果tag有数据并且first_mb_in_slice=0表示有上一针*/
            if (sliceHeader.first_mb_in_slice == 0 && picture->useFlag) {
                /*计算pts和dts*/
                computedTimestamp(picture);

                /*
                    * 参考图片标记过程
                    * 当前图片的所有SLICE都被解码。
                    * 参考8.2.5.1第一条规则
                    * */
                /*这里标记了这个使用的帧是长期参考还是短期参考，并且给出一个空闲的帧*/
                ret = gop.decoding_finish(picture, unoccupiedPicture);
                if (ret < 0) {
                    fprintf(stderr, "图像解析失败\n");
                    return ret;
                }
                /*设置这个空闲的帧的slice header*/
                unoccupiedPicture->sliceHeader = sliceHeader;
                unoccupiedPicture->useFlag = true;

                /*解码POC*/
                unoccupiedPicture->decoding_process_for_picture_order_count();

                /*参考帧重排序在每个P、SP或B片的解码过程开始时调用。*/
                if (sliceHeader.slice_type == H264_SLIECE_TYPE_P ||
                    sliceHeader.slice_type == H264_SLIECE_TYPE_SP ||
                    sliceHeader.slice_type == H264_SLIECE_TYPE_B) {
                    gop.decoding_process_for_reference_picture_lists_construction(unoccupiedPicture);
                }
                unoccupiedPicture->size += tempSize;
                unoccupiedPicture->data.push_back({tempSize, buf, 0});
                /*所有数据获取完毕*/
                if (!flag) {
                    /*最后一帧*/
                    finishFlag = true;
                    flag = true;
                }

                return 0;
            }

            picture->sliceHeader = sliceHeader;
            if (sliceHeader.first_mb_in_slice == 0) {
                /*表示这帧正在使用，不要释放*/
                picture->useFlag = true;

                /*解码POC,查看h264文档 8 Decoding process (only needed to be invoked for one slice of a picture)只需要为一帧的切片调用即可*/
                picture->decoding_process_for_picture_order_count();

                /*参考帧重排序在每个P、SP或B片的解码过程开始时调用。*/
                if (sliceHeader.slice_type == H264_SLIECE_TYPE_P ||
                    sliceHeader.slice_type == H264_SLIECE_TYPE_SP ||
                    sliceHeader.slice_type == H264_SLIECE_TYPE_B) {
                    gop.decoding_process_for_reference_picture_lists_construction(picture);
                }
            }
            picture->size += tempSize;
            picture->data.push_back({tempSize, buf, 0});
        } else if (nalUnitHeader.nal_unit_type == H264_NAL_SEI) {
            //++i;
        } else {
            fprintf(stderr, "不支持解析 nal_unit_type = %d\n", nalUnitHeader.nal_unit_type);
        }
    }
    return 0;
}

int NALReader::test(NALPicture *&picture, bool &flag) {


    int ret;
    if (finishFlag) {
        flag = false;
        /*计算pts和dts*/
        computedTimestamp(unoccupiedPicture);
        picture = unoccupiedPicture;

        gop.reset();
        return 0;
    }

    uint8_t *data = nullptr;
    uint32_t size = 0;
    int startCodeLength = 0;

    while (!pictureFinishFlag) {
        /*每次读取一个nalu*/
        ret = readNalUint(data, size, startCodeLength, flag);
        if (ret < 0) {
            fprintf(stderr, "读取nal uint发生错误\n");
            return ret;
        }

        ret = putNalUintData(picture, data, size);
        if (ret < 0) {
            fprintf(stderr, "putNalUintData失败\n");
            return ret;
        }

        if (!flag) {
            /*最后一帧*/
            finishFlag = true;
            flag = true;
        }

    }
    return 0;
}

int NALReader::putNalUintData(NALPicture *&picture, uint8_t *data, uint32_t size) {
    int ret;

    if (pictureFinishFlag) {
        picture->useFlag = false;
        picture = unoccupiedPicture;
        pictureFinishFlag = false;
    }

    /*读取nalu header*/
    nalUnitHeader.nal_unit(data[0]);
    if (nalUnitHeader.nal_unit_type == H264_NAL_SPS) {
        const uint32_t tempSize = size;
        uint8_t *buf = new uint8_t[tempSize]; // NOLINT(modernize-use-auto)
        memcpy(buf, data, tempSize);

        /*去除防竞争字节*/
        NALHeader::ebsp_to_rbsp(data, size);
        ReadStream rs(data, size);

        sps.seq_parameter_set_data(rs);
        spsList[sps.seq_parameter_set_id] = sps;

        if (picture->useFlag) {
            /*计算pts和dts*/
            computedTimestamp(picture);
            /*
               * 参考图片标记过程
               * 当前图片的所有SLICE都被解码。
               * 参考8.2.5.1第一条规则
               * */
            /*这里标记了这个使用的帧是长期参考还是短期参考，并且给出一个空闲的帧*/
            ret = gop.decoding_finish(picture, unoccupiedPicture);
            if (ret < 0) {
                fprintf(stderr, "图像解析失败\n");
                return ret;
            }

            unoccupiedPicture->size += tempSize;
            unoccupiedPicture->data.push_back({tempSize, buf, 0});
            return 0;
        }

        picture->size += tempSize;
        picture->data.push_back({tempSize, buf, 1});

    } else if (nalUnitHeader.nal_unit_type == H264_NAL_PPS) {
        const uint32_t tempSize = size;
        uint8_t *buf = new uint8_t[tempSize]; // NOLINT(modernize-use-auto)
        memcpy(buf, data, tempSize);


        NALHeader::ebsp_to_rbsp(data, size);
        ReadStream rs(data, size);


        pps.pic_parameter_set_rbsp(rs, spsList);
        ppsList[pps.pic_parameter_set_id] = pps;
        if (picture->useFlag) {
            computedTimestamp(picture);
            /*
                * 参考图片标记过程
                * 当前图片的所有SLICE都被解码。
                * 参考8.2.5.1第一条规则
                * */
            /*这里标记了这个使用的帧是长期参考还是短期参考，并且给出一个空闲的帧*/
            ret = gop.decoding_finish(picture, unoccupiedPicture);
            if (ret < 0) {
                fprintf(stderr, "图像解析失败\n");
                return ret;
            }

            unoccupiedPicture->size += tempSize;
            unoccupiedPicture->data.push_back({tempSize, buf, 0});
            return 0;
        }

        picture->size += tempSize;
        picture->data.push_back({tempSize, buf, 2});
    } else if (nalUnitHeader.nal_unit_type == H264_NAL_SLICE || nalUnitHeader.nal_unit_type == H264_NAL_IDR_SLICE) {
        const uint32_t tempSize = size;
        uint8_t *buf = new uint8_t[tempSize]; // NOLINT(modernize-use-auto)
        memcpy(buf, data, tempSize);


        NALHeader::ebsp_to_rbsp(data, size);
        ReadStream rs(data, size);

//        NALSliceHeader sliceHeader;
        sliceHeader.slice_header(rs, nalUnitHeader, spsList, ppsList);

        /*先处理上一帧，如果tag有数据并且first_mb_in_slice=0表示有上一针*/
        if (sliceHeader.first_mb_in_slice == 0 && picture->useFlag) {
            /*计算pts和dts*/
            computedTimestamp(picture);
            /*
                * 参考图片标记过程
                * 当前图片的所有SLICE都被解码。
                * 参考8.2.5.1第一条规则
                * */
            /*这里标记了这个使用的帧是长期参考还是短期参考，并且给出一个空闲的帧*/
            ret = gop.decoding_finish(picture, unoccupiedPicture);
            if (ret < 0) {
                fprintf(stderr, "图像解析失败\n");
                return ret;
            }
            /*设置这个空闲的帧的slice header*/
            unoccupiedPicture->sliceHeader = sliceHeader;
            unoccupiedPicture->useFlag = true;

            /*解码POC，只需要为一帧的一个切片调用*/
            unoccupiedPicture->decoding_process_for_picture_order_count();

            /*参考帧重排序在每个P、SP或B片的解码过程开始时调用。*/
            if (sliceHeader.slice_type == H264_SLIECE_TYPE_P ||
                sliceHeader.slice_type == H264_SLIECE_TYPE_SP ||
                sliceHeader.slice_type == H264_SLIECE_TYPE_B) {
                gop.decoding_process_for_reference_picture_lists_construction(unoccupiedPicture);
            }
            unoccupiedPicture->size += tempSize;
            unoccupiedPicture->data.push_back({tempSize, buf, 0});

            pictureFinishFlag = true;
            return 0;
        }

        picture->sliceHeader = sliceHeader;

        if (sliceHeader.first_mb_in_slice == 0) {
            /*表示这帧正在使用，不要释放*/
            picture->useFlag = true;

            /*解码POC,查看h264文档 8 Decoding process (only needed to be invoked for one slice of a picture)只需要为一帧的切片调用即可*/
            picture->decoding_process_for_picture_order_count();

            /*参考帧重排序在每个P、SP或B片的解码过程开始时调用。*/
            if (sliceHeader.slice_type == H264_SLIECE_TYPE_P ||
                sliceHeader.slice_type == H264_SLIECE_TYPE_SP ||
                sliceHeader.slice_type == H264_SLIECE_TYPE_B) {
                gop.decoding_process_for_reference_picture_lists_construction(picture);
            }
        }
        picture->size += tempSize;
        picture->data.push_back({tempSize, buf, 0});
    }
    return 0;
}

void NALReader::computedTimestamp(NALPicture *picture) {
    if (picture->pictureOrderCount == 0) {
        videoDecodeIdrFrameNumber = videoDecodeFrameNumber * 2;
    }

    picture->pts = av_rescale_q((videoDecodeIdrFrameNumber + picture->pictureOrderCount) / 2,
                                picture->sliceHeader.sps.timeBase,
                                {1, 90000});
    picture->dts = av_rescale_q(videoDecodeFrameNumber, picture->sliceHeader.sps.timeBase, {1, 90000});
    picture->pcr = av_rescale_q(videoDecodeFrameNumber, picture->sliceHeader.sps.timeBase, {1, 1000});


    ++videoDecodeFrameNumber;
    picture->duration = (double) videoDecodeFrameNumber / picture->sliceHeader.sps.fps;
}

NALPicture *NALReader::allocPicture() {

    for (NALPicture *pic: gop.dpb) {
        if (!pic->useFlag && pic->reference_marked_type == UNUSED_FOR_REFERENCE) {
            return pic;
        }
    }
    return nullptr;
}


NALReader::~NALReader() {
    fs.close();
    if (bufferStart) {
        delete[] bufferStart;
        bufferStart = nullptr;
    }


    delete[] spsData;


    delete[] ppsData;

}



















