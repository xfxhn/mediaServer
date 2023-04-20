#include "NALReader.h"
#include "bitStream/readStream.h"
#include "log/logger.h"


int NALReader::init() {

    bufferStart = new uint8_t[MAX_BUFFER_SIZE];
    blockBufferSize = 0;
    return 0;
}


int NALReader::findNALU(uint8_t *&pos1, uint8_t *&pos2, int &startCodeLen1, int &startCodeLen2) const {

    if (!bufferStart) {
        log_error("请初始化");
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


int NALReader::getVideoFrame3(NALPicture *&picture) {
    int ret;

    uint8_t *pos1 = nullptr;
    uint8_t *pos2 = nullptr;
    int startCodeLen1 = 0;
    int startCodeLen2 = 0;

    /*找到了nalu，判断是否是完整的一帧*/
//    if (picture->pictureFinishFlag) {
//        picture = unoccupiedPicture;
//    }
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

int NALReader::getVideoFrame2(NALPicture *&picture, uint8_t *data, uint32_t size, uint8_t startCodeLength) {

    /*能够进入到这个函数，肯定是一个完整的nalu了 */
    if (picture->pictureFinishFlag) {
        /*进入到这里才是完整的一帧*/
        picture = unoccupiedPicture;
    }

    int ret = disposePicture(picture, data, size, startCodeLength);
    if (ret < 0) {
        log_error("disposePicture 错误");
        return ret;
    }
    return 0;
}

int NALReader::disposePicture(NALPicture *picture, uint8_t *data, uint32_t size, uint8_t startCodeLength) {
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
    } else if (nalUnitHeader.nal_unit_type == H264_NAL_SLICE || nalUnitHeader.nal_unit_type == H264_NAL_IDR_SLICE) {

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
            log_error("不支持一帧分成多个slice");
            return -1;
        }
        /*解码POC,查看h264文档 8 Decoding process (only needed to be invoked for one slice of a picture)只需要为一帧的切片调用即可*/
        ret = picture->decoding_process_for_picture_order_count();
        if (ret < 0) {
            log_error("解析picture order count 失败");
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
            log_error("图像解析失败");
            return ret;
        }
        /*计算pts和dts*/
        computedTimestamp(picture);

        if (nalUnitHeader.nal_unit_type == H264_NAL_IDR_SLICE) {
            /*为每个IDR帧前面添加sps和pps*/
            picture->size += spsSize;
            picture->data.push_back({spsSize, spsData, 1});

            picture->size += ppsSize;
            picture->data.push_back({ppsSize, ppsData, 2});
        }
        picture->size += size;
        picture->data.push_back({size, data, 0});

        picture->pictureFinishFlag = true;
        picture->finishFlag = true;
    } else {
        log_warn("其他type");
        picture->pictureFinishFlag = false;
        picture->finishFlag = false;
    }
    return 0;
}


void NALReader::computedTimestamp(NALPicture *picture) {
    if (picture->pictureOrderCount == 0) {
        videoDecodeIdrFrameNumber = videoDecodeFrameNumber * 2;
    }
    picture->decodeFrameNumber = videoDecodeFrameNumber;
    picture->pts = av_rescale_q((videoDecodeIdrFrameNumber + picture->pictureOrderCount) / 2,
                                picture->sliceHeader.sps.timeBase,
                                {1, 1000});
    picture->dts = av_rescale_q(videoDecodeFrameNumber, picture->sliceHeader.sps.timeBase, {1, 1000});
    picture->pcr = av_rescale_q(videoDecodeFrameNumber, picture->sliceHeader.sps.timeBase, {1, 1000});

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
    delete[] bufferStart;
}




























