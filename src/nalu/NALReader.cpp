#include "NALReader.h"
#include "readStream.h"


void NALReader::reset() {
    fs.seekg(0);
    fs.read(reinterpret_cast<char *>(bufferStart), MAX_BUFFER_SIZE);
    blockBufferSize = fs.gcount();
    bufferPosition = bufferStart;
    bufferEnd = bufferStart + blockBufferSize - 1;


    videoDecodeFrameNumber = 0;
    videoDecodeIdrFrameNumber = 0;
    unoccupiedPicture = nullptr;
    gop.reset();
    finishFlag = false;
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

int NALReader::init1(const std::string &dir, uint32_t transportStreamPacketNumber, int timestamp) {
    clockRate = timestamp;
    currentPacket = transportStreamPacketNumber;
    std::string name = "test" + std::to_string(currentPacket) + ".ts";
    printf("读取%s文件 video init\n", name.c_str());
    fs.open("test/" + name, std::ios::out | std::ios::binary);
    if (!fs.is_open()) {
        fprintf(stderr, "open %s failed\n", name.c_str());
        return -1;
    }

    bufferStart = new uint8_t[MAX_BUFFER_SIZE];

    blockBufferSize = 0;

    return 0;
}

int NALReader::getTransportStreamData() {
    int ret;

    while (true) {
        fs.read(reinterpret_cast<char *>(transportStreamBuffer), TRANSPORT_STREAM_PACKETS_SIZE);
        uint32_t size = fs.gcount();
        // bufferPosition += size;
        if (size != TRANSPORT_STREAM_PACKETS_SIZE) {
            /*表示这个文件读完了，读下一个*/
            /*这里ts文件，应该就是188的倍数，不是188的倍数，这个文件是有问题*/
            fs.close();
            std::string name = "test" + std::to_string(++currentPacket) + ".ts";
            printf("读取%s文件 video\n", name.c_str());
            fs.open("test/" + name, std::ios::out | std::ios::binary);
            if (!fs.is_open()) {
                fprintf(stderr, "open %s failed\n", name.c_str());
                return -1;
            }

            fs.read(reinterpret_cast<char *>(transportStreamBuffer), TRANSPORT_STREAM_PACKETS_SIZE);
            size = fs.gcount();
            if (size != TRANSPORT_STREAM_PACKETS_SIZE) {
                fprintf(stderr, "没读到一个ts包的大小，read size = %d\n", size);
                return -1;
            }
        }
        ReadStream rs(transportStreamBuffer, size);
        ret = demux.readFrame(rs);
        if (ret < 0) {
            fprintf(stderr, "demux.readVideoFrame失败\n");
            return ret;
        }
        if (ret == VIDEO_PID) {
            size = rs.size - rs.position;
            memcpy(bufferStart + blockBufferSize, rs.currentPtr, size);
            blockBufferSize += size;
            bufferEnd = bufferStart + blockBufferSize;
            break;
        }

    }

    return 0;
}

int NALReader::readNalUint1(uint8_t *&data, uint32_t &size) {
    int ret;
    uint8_t *pos1 = nullptr;
    uint8_t *pos2 = nullptr;
    int startCodeLen1 = 0;
    int startCodeLen2 = 0;

    /*还剩多少字节未读取*/
    uint32_t remainingByte = bufferEnd - bufferPosition;
    /*这里内存重叠了，不过是正常拷贝过去的*/
    memcpy(bufferStart, bufferPosition, remainingByte);
    blockBufferSize = remainingByte;
    bufferEnd = bufferStart + remainingByte;

    while (true) {
        ret = findNALU(pos1, pos2, startCodeLen1, startCodeLen2);

        /*如果没找到就要继续往buffer里面塞数据，直到找到为止*/
        if (ret == 1) { //表示找到了开头的startCode,没找到后面的
            ret = getTransportStreamData();
            if (ret < 0) {
                fprintf(stderr, "获取ts video数据失败\n");
                return ret;
            }
        } else if (ret == 2) {//都找到了
            data = pos1 + startCodeLen1;
            size = pos2 - data;
            bufferPosition = pos2;
            break;
        } else {
            //错误
            fprintf(stderr, "没有找到开头startCode，也没有找到后面的startCode\n");
            return -1;
        }
    }
    return 0;
}

int NALReader::findNALU(uint8_t *&pos1, uint8_t *&pos2, int &startCodeLen1, int &startCodeLen2) {

    if (!bufferStart) {
        fprintf(stderr, "请初始化\n");
        return -1;
    }

    if (blockBufferSize == 0) {
        return 1;
    }
    startCodeLen1 = 0;
    startCodeLen2 = 0;
    uint32_t pos = 0;
    while (pos + 3 < blockBufferSize) {
        if (bufferStart[pos] == 0 && bufferStart[pos + 1] == 0 && bufferStart[pos + 2] == 1) {
            startCodeLen1 = 3;
            break;
        } else if (bufferStart[pos] == 0 && bufferStart[pos + 1] == 0 && bufferStart[pos + 2] == 0 &&
                   bufferStart[pos + 3] == 1) {
            startCodeLen1 = 4;
            break;
        }
        pos++;
    }
    pos1 = &bufferStart[pos];
    if (startCodeLen1 == 0) {
        fprintf(stderr, "没找到start code\n");
        return -1;
    }

    pos = pos + startCodeLen1;
    while (pos + 3 < blockBufferSize) {
        if (bufferStart[pos] == 0 && bufferStart[pos + 1] == 0 && bufferStart[pos + 2] == 1) {
            startCodeLen2 = 3;
            break;
        } else if (bufferStart[pos] == 0 && bufferStart[pos + 1] == 0 && bufferStart[pos + 2] == 0 &&
                   bufferStart[pos + 3] == 1) {
            startCodeLen2 = 4;
            break;
        }
        pos++;
    }
    pos2 = &bufferStart[pos];

    return startCodeLen2 == 0 ? 1 : 2;
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

        if (type == 1) { //表示找到了开头的startCode,没找到后面的
            /*还剩多少字节未读取*/
            uint32_t residual = (bufferEnd - pos1 + 1);
            /*已经读取了多少个字节*/
            uint32_t readSize = blockBufferSize - residual;

            memcpy(bufferStart, pos1, residual);
            //每次读File::MAX_BUFFER_SIZE个，这里读取的NALU必须要包含一整个slice
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
    if (end == nullptr) {
        return 1;
    }
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

int NALReader::getVideoFrame1(NALPicture *&picture) {
    int ret;

    uint8_t *data;
    uint32_t size;

    ret = readNalUint1(data, size);
    if (ret < 0) {
        fprintf(stderr, "读取nalu单元错误\n");
        return ret;
    }

    ret = test1(picture, data, size, 0);
    if (ret < 0) {
        fprintf(stderr, "计算picture错误\n");
        return ret;
    }
    if (!picture->pictureFinishFlag) {
        this->getVideoFrame1(picture);
    }

    return 0;
}



int NALReader::test1(NALPicture *&picture, uint8_t *data, uint32_t size, uint8_t startCodeLength) {
    int ret;

    //   uint8_t num = flag ? 4 : 0;
    if (picture->pictureFinishFlag) {
        picture = unoccupiedPicture;
    }

    nalUnitHeader.nal_unit(data[startCodeLength]);
    if (nalUnitHeader.nal_unit_type == H264_NAL_SPS) {
        /*有起始码的*/
        memcpy(spsData, data, size);
        spsSize = size;


        size -= startCodeLength;
        /*去除防竞争字节*/
        uint8_t *ebsp = data + startCodeLength;
        NALHeader::ebsp_to_rbsp(ebsp, size);
        ReadStream rs(ebsp, size);


        sps.seq_parameter_set_data(rs);
        spsList[sps.seq_parameter_set_id] = sps;
        picture->pictureFinishFlag = false;
        /*picture->size += size;
        picture->data.push_back({size, data, 0});*/
    } else if (nalUnitHeader.nal_unit_type == H264_NAL_PPS) {


        /*有起始码的*/
        memcpy(ppsData, data, size);
        ppsSize = size;

        size -= startCodeLength;
        uint8_t *ebsp = data + startCodeLength;
        NALHeader::ebsp_to_rbsp(ebsp, size);
        ReadStream rs(ebsp, size);


        pps.pic_parameter_set_rbsp(rs, spsList);
        ppsList[pps.pic_parameter_set_id] = pps;
        picture->pictureFinishFlag = false;
        /* picture->size += size;
         picture->data.push_back({size, data, 2});*/
    } else if (nalUnitHeader.nal_unit_type == H264_NAL_SLICE) {

        /*
         * 这里只对一帧的前30个字节进行去除防竞争字节，然后去解码slice header
         * 但是这里有可能slice header 30个字节不够，导致解析错误，概率很小很小，一般slice header不足30个字节
         * 或者一帧数据还不够30个字节，也不太可能
         * */
        uint32_t headerSize = 30;
        /*todo 这里可以优化，放到类里面，避免多次分配释放*/
        uint8_t header[30];
        memcpy(header, data + startCodeLength, headerSize);
        NALHeader::ebsp_to_rbsp(header, headerSize);
        ReadStream rs(header, headerSize);


        picture->sliceHeader.slice_header(rs, nalUnitHeader, spsList, ppsList);
        if (picture->sliceHeader.first_mb_in_slice != 0) {
            fprintf(stderr, "不支持一帧分成多个slice\n");
            return -1;
        }
        /*解码POC,查看h264文档 8 Decoding process (only needed to be invoked for one slice of a picture)只需要为一帧的切片调用即可*/
        ret = picture->decoding_process_for_picture_order_count();
        if (ret < 0) {
            fprintf(stderr, "解析picture order count 失败\n");
            return ret;
        }
        /*参考帧重排序在每个P、SP或B片的解码过程开始时调用。*/
        if (picture->sliceHeader.slice_type == H264_SLIECE_TYPE_P ||
            picture->sliceHeader.slice_type == H264_SLIECE_TYPE_SP ||
            picture->sliceHeader.slice_type == H264_SLIECE_TYPE_B) {
            gop.decoding_process_for_reference_picture_lists_construction(picture);
        }
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
        /*计算pts和dts*/
        computedTimestamp(picture);

        picture->size += size;
        picture->data.push_back({size, data, 0});

        picture->pictureFinishFlag = true;
    } else if (nalUnitHeader.nal_unit_type == H264_NAL_IDR_SLICE) {


        uint32_t headerSize = 30;
        uint8_t header[30];
        memcpy(header, data + startCodeLength, headerSize);
        NALHeader::ebsp_to_rbsp(header, headerSize);
        ReadStream rs(header, headerSize);


        picture->sliceHeader.slice_header(rs, nalUnitHeader, spsList, ppsList);
        if (picture->sliceHeader.first_mb_in_slice != 0) {
            fprintf(stderr, "不支持一帧分成多个slice\n");
            return -1;
        }

        /*解码POC,查看h264文档 8 Decoding process (only needed to be invoked for one slice of a picture)只需要为一帧的切片调用即可*/
        ret = picture->decoding_process_for_picture_order_count();
        if (ret < 0) {
            fprintf(stderr, "解析picture order count 失败\n");
            return ret;
        }
        /*参考帧重排序在每个P、SP或B片的解码过程开始时调用。*/
        if (picture->sliceHeader.slice_type == H264_SLIECE_TYPE_P ||
            picture->sliceHeader.slice_type == H264_SLIECE_TYPE_SP ||
            picture->sliceHeader.slice_type == H264_SLIECE_TYPE_B) {
            gop.decoding_process_for_reference_picture_lists_construction(picture);
        }

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
        /*计算pts和dts*/
        computedTimestamp(picture);


        picture->size += spsSize;
        picture->data.push_back({spsSize, spsData, 1});


        picture->size += ppsSize;
        picture->data.push_back({ppsSize, ppsData, 2});

        picture->size += size;
        picture->data.push_back({size, data, 3});
        picture->pictureFinishFlag = true;
    } else {
        fprintf(stderr, "其他type\n");
        picture->pictureFinishFlag = false;
    }
    return 0;
}


void NALReader::computedTimestamp(NALPicture *picture) {
    if (picture->pictureOrderCount == 0) {
        videoDecodeIdrFrameNumber = videoDecodeFrameNumber * 2;
    }

    picture->pts = av_rescale_q((videoDecodeIdrFrameNumber + picture->pictureOrderCount) / 2,
                                picture->sliceHeader.sps.timeBase,
                                {1, clockRate});
    picture->dts = av_rescale_q(videoDecodeFrameNumber, picture->sliceHeader.sps.timeBase, {1, clockRate});
    picture->pcr = av_rescale_q(videoDecodeFrameNumber, picture->sliceHeader.sps.timeBase, {1, clockRate});
    /*转换成微秒*/
    picture->interval = 1000000 / (int) picture->sliceHeader.sps.fps;
    picture->duration = (double) videoDecodeFrameNumber / picture->sliceHeader.sps.fps;
    ++videoDecodeFrameNumber;
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


}

























