#include "NALReader.h"
#include "readStream.h"
#include <thread>

void NALReader::reset() {
    fs.seekg(0);
    blockBufferSize = 0;
    videoDecodeFrameNumber = 0;
    videoDecodeIdrFrameNumber = 0;
    unoccupiedPicture = nullptr;
    bufferEnd = nullptr;
    bufferPosition = nullptr;

    gop.reset();
}

int NALReader::init2() {

    bufferStart = new uint8_t[MAX_BUFFER_SIZE];
    blockBufferSize = 0;
    return 0;
}

int NALReader::init1(const std::string &dir, uint32_t transportStreamPacketNumber, int timestamp) {
    path = dir;
    clockRate = timestamp;
    currentPacket = transportStreamPacketNumber;
    std::string name = "/test" + std::to_string(currentPacket) + ".ts";
    printf("读取%s文件 video init\n", name.c_str());
    fs.open(path + name, std::ios::out | std::ios::binary);
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
    std::string name;
    //uint8_t offset = 0;
    uint32_t size = 0;
    while (true) {
        fs.read(reinterpret_cast<char *>(transportStreamBuffer + size), TRANSPORT_STREAM_PACKETS_SIZE - size);
        size += fs.gcount();
        if (size == 0) {
            uint32_t pos = fs.tellg();
            /*表示这个文件读完了，读下一个*/
            fs.close();
            name = "/test" + std::to_string(++currentPacket) + ".ts";
            printf("读取%s文件 读取的size = %d video\n", name.c_str(), size);
            fs.open(path + name, std::ios::out | std::ios::binary);
            if (!fs.is_open()) {
                fprintf(stderr, "读取%s失败 video\n", name.c_str());
                /*如果是走到这里打开下一个文件失败，表示上个文件还没读完，继续读上个文件*/
                name = "/test" + std::to_string(--currentPacket) + ".ts";
                fs.open(path + name, std::ios::out | std::ios::binary);
                /*返回到上次读取到的位置*/
                fs.seekg(pos);
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
                continue;
            }

            fs.read(reinterpret_cast<char *>(transportStreamBuffer), TRANSPORT_STREAM_PACKETS_SIZE);
            size = fs.gcount();
            if (size != TRANSPORT_STREAM_PACKETS_SIZE) {
                fprintf(stderr, "没读到一个ts包的大小，read size = %d\n", size);
                return -1;
            }
        } else if (size < TRANSPORT_STREAM_PACKETS_SIZE) {
            fs.clear();
            fprintf(stderr, "size < TRANSPORT_STREAM_PACKETS_SIZE %d, video\n", size);
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            continue;
        }
        /*走到这里肯定有188个字节*/
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
        size = 0;
    }

    return 0;
}


int NALReader::readNalUint(uint8_t *&data, uint32_t &size) {
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


int NALReader::findNALU(uint8_t *&pos1, uint8_t *&pos2, int &startCodeLen1, int &startCodeLen2) const {

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


int NALReader::getVideoParameter() {
    int ret;

    uint8_t *data;
    uint32_t size;

    bool flag1 = true;
    bool flag2 = true;
    while (flag1 || flag2) {
        ret = readNalUint(data, size);
        if (ret < 0) {
            fprintf(stderr, "读取nalu单元错误\n");
            return ret;
        }
        nalUnitHeader.nal_unit(data[0]);
        if (nalUnitHeader.nal_unit_type == H264_NAL_SPS) {
            memcpy(spsData, data, size);
            spsSize = size;

            //uint8_t *ebsp = data + startCodeLength;
            NALHeader::ebsp_to_rbsp(data, size);
            ReadStream rs(data, size);
            sps.seq_parameter_set_data(rs);
            spsList[sps.seq_parameter_set_id] = sps;
            flag1 = false;
        } else if (nalUnitHeader.nal_unit_type == H264_NAL_PPS) {
            memcpy(ppsData, data, size);
            ppsSize = size;

            NALHeader::ebsp_to_rbsp(data, size);
            ReadStream rs(data, size);
            pps.pic_parameter_set_rbsp(rs, spsList);
            flag2 = false;
        }
    }
    reset();
    return 0;
}

int NALReader::getParameter() {
    int ret;

    uint8_t *pos1 = nullptr;
    uint8_t *pos2 = nullptr;
    int startCodeLen1 = 0;
    int startCodeLen2 = 0;


    ret = findNALU(pos1, pos2, startCodeLen1, startCodeLen2);
    if (ret == 2) {
        /*表示找到一个nalu*/
        uint8_t *data = pos1 + startCodeLen1;
        uint32_t size = pos2 - data;

        nalUnitHeader.nal_unit(data[0]);
        if (nalUnitHeader.nal_unit_type == H264_NAL_SPS) {
            memcpy(spsData, data, size);
            spsSize = size;

            NALHeader::ebsp_to_rbsp(data, size);
            ReadStream rs(data, size);
            sps.seq_parameter_set_data(rs);
            spsList[sps.seq_parameter_set_id] = sps;
            ret = 3;
        } else if (nalUnitHeader.nal_unit_type == H264_NAL_PPS) {
            memcpy(ppsData, data, size);
            ppsSize = size;

            NALHeader::ebsp_to_rbsp(data, size);
            ReadStream rs(data, size);
            pps.pic_parameter_set_rbsp(rs, spsList);
            ret = 4;
        }
        resetFlag = true;
        bufferPosition = pos2;

//        if (picture->finishFlag == false) {
//            resetBuffer();
//        }

    }

    return ret;
}

void NALReader::putData(uint8_t *data, uint32_t size) {
    memcpy(bufferStart + blockBufferSize, data, size);
    blockBufferSize += size;
    bufferEnd = bufferStart + blockBufferSize;
}

void NALReader::resetBuffer() {
    /*有可能找到了，但是bufferEnd - bufferPosition刚好=0*/
    if (resetFlag) {
        uint32_t remainingByte = bufferEnd - bufferPosition;
        blockBufferSize = remainingByte;

        memcpy(bufferStart, bufferPosition, remainingByte);
        resetFlag = false;
    }

}


int NALReader::getVideoFrame1(NALPicture *&picture) {
    int ret;

    uint8_t *data;
    uint32_t size;
    if (picture->pictureFinishFlag) {
        picture = unoccupiedPicture;
    }

    while (!picture->pictureFinishFlag) {
        ret = readNalUint(data, size);
        if (ret < 0) {
            fprintf(stderr, "读取nalu单元错误\n");
            return ret;
        }

        ret = test1(picture, data, size, 0);
        if (ret < 0) {
            fprintf(stderr, "计算picture错误\n");
            return ret;
        }
    }


    return 0;
}

int NALReader::getVideoFrame2(NALPicture *&picture, uint8_t *data, uint32_t size, uint8_t startCodeLength) {
    if (picture->pictureFinishFlag) {
        picture = unoccupiedPicture;
    }


    int ret = test1(picture, data, size, startCodeLength);
    if (ret < 0) {
        fprintf(stderr, "计算picture错误\n");
        return ret;
    }
    return 0;
}

int NALReader::getVideoFrame3(NALPicture *&picture) {
    int ret;

    uint8_t *pos1 = nullptr;
    uint8_t *pos2 = nullptr;
    int startCodeLen1 = 0;
    int startCodeLen2 = 0;


    ret = findNALU(pos1, pos2, startCodeLen1, startCodeLen2);
    if (ret == 2) {
        uint8_t *data = pos1 + startCodeLen1;
        uint32_t size = pos2 - data;

        ret = getVideoFrame2(picture, data, size, 0);
        if (ret < 0) {
            return ret;
        }
        /*找到一个nalu，才可以去重置这个内存*/
        resetFlag = true;
        bufferPosition = pos2;
        if (picture->finishFlag == false) {
            resetBuffer();
        }
    }
    return ret;
}

int NALReader::test1(NALPicture *picture, uint8_t *data, uint32_t size, uint8_t startCodeLength) {
    int ret;

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
        picture->finishFlag = false;
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
        picture->finishFlag = false;
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
        picture->finishFlag = true;
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
        picture->finishFlag = true;
    } else {
        fprintf(stderr, "其他type\n");
        picture->pictureFinishFlag = false;
        picture->finishFlag = false;
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
    picture->interval = 1000 / (int) picture->sliceHeader.sps.fps;
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



























