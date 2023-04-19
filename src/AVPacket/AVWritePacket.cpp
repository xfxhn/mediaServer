
#include "AVWritePacket.h"

#include <filesystem>
#include "log/logger.h"

int AVWritePacket::init(const std::string &dir, uint32_t transportStreamPacketNumber) {
    ts.init();

    path = dir;
    /*创建目录*/
    /*返回true表示创建成功，返回false表示已经存在*/
    std::filesystem::create_directories(path);

    currentPacket = transportStreamPacketNumber;
    m3u8FileSystem.open(dir + "/test.m3u8", std::ios::binary | std::ios::out | std::ios::trunc);
    if (!m3u8FileSystem.is_open()) {
        fprintf(stderr, "cloud not open %s\n", (dir + "/test.m3u8").c_str());
        return -1;
    }

    videoReader.init();
    audioReader.init();
    picture = videoReader.allocPicture();
    return 0;
}

int AVWritePacket::writeFrame(uint8_t *data, uint32_t size, const std::string &type, bool flag) {
    int ret;
    if (type == "video") {
        memcpy(videoReader.bufferStart + videoReader.blockBufferSize, data, size);
        videoReader.blockBufferSize += size;
        if (flag) {
            ret = videoReader.getVideoFrame2(picture, videoReader.bufferStart, videoReader.blockBufferSize, 4);
            if (ret < 0) {
                log_error("videoReader.getVideoFrame2 错误");
                return ret;
            }

            if (picture->pictureFinishFlag) {
                ret = writeTransportStream();
                if (ret < 0) {
                    log_error("writeTransportStream 错误");
                    return ret;
                }
            }
            videoReader.blockBufferSize = 0;
        }

    } else if (type == "audio") {
        AdtsHeader::setFrameLength((int) size);
        memcpy(data, AdtsHeader::header, 7);
        audioReader.disposeAudio(header, data, size);
        ret = ts.writeAudioFrame(header, transportStreamFileSystem);
        if (ret < 0) {
            log_error("写入音频失败");
            return -1;
        }
    }


    return 0;
}

int AVWritePacket::writeTransportStream() {
    int ret;
    if (picture->sliceHeader.nalu.IdrPicFlag) {
        /*set ts packet = 0*/
        ts.resetPacketSize();

        transportStreamFileSystem.close();
        std::string name = "test" + std::to_string(currentPacket++) + ".ts";
        log_info("写入%s文件", name.c_str());
        double duration = picture->duration - lastDuration;
        list.push_back({name, duration});
        transportStreamFileSystem.open(path + "/" + name, std::ios::binary | std::ios::out | std::ios::trunc);
        if (!transportStreamFileSystem.is_open()) {
            log_error("打开%s文件失败", name.c_str());
            return -1;
        }
        ts.writeTable(transportStreamFileSystem);

        if (list.size() > 3) {
            char m3u8Buffer[512]{0};
            TransportStreamInfo info1 = list[list.size() - 1];
            TransportStreamInfo info2 = list[list.size() - 2];
            TransportStreamInfo info3 = list[list.size() - 3];
            double maxDuration = std::max(info3.duration, std::max(info1.duration, info2.duration));

            m3u8FileSystem.seekp(0, std::ios::beg);
            m3u8FileSystem.write("", 0);

            sprintf(m3u8Buffer,
                    "#EXTM3U\r\n"
                    "#EXT-X-VERSION:3\r\n"
                    "#EXT-X-TARGETDURATION:%f\r\n"
                    "#EXT-X-MEDIA-SEQUENCE:%d\r\n"
                    "#EXTINF:%f,\r\n"
                    "%s\r\n"
                    "#EXTINF:%f,\r\n"
                    "%s\r\n"
                    "#EXTINF:%f,\r\n"
                    "%s\r\n",
                    maxDuration,
                    seq++,
                    info3.duration,
                    info3.name.c_str(),
                    info2.duration,
                    info2.name.c_str(),
                    info1.duration,
                    info1.name.c_str()
            );
            m3u8FileSystem.write(m3u8Buffer, (int) strlen(m3u8Buffer));
        }
        if (list.size() > 10) {
            std::string &oldName = list[0].name;
            ret = std::filesystem::remove(path + "/" + oldName);
            if (!ret) {
                log_error("删除%s失败", oldName.c_str());
                return ret;
            }
            list.erase(list.begin());
        }
        lastDuration = picture->duration;
    }

    ret = ts.writeVideoFrame(picture, transportStreamFileSystem);
    if (ret < 0) {
        log_error("写入视频帧失败");
        return ret;
    }
    return 0;
}

int AVWritePacket::setVideoParameter(uint8_t *data, uint32_t size) {
    return videoReader.getVideoFrame2(picture, data, size, 4);
}

int AVWritePacket::setAudioParameter(uint8_t audioObjectType, uint8_t samplingFrequencyIndex,
                                     uint8_t channelConfiguration) {
    WriteStream ws(AdtsHeader::header, 7);
    header.setConfig(audioObjectType, samplingFrequencyIndex, channelConfiguration, 0);
    header.writeAdtsHeader(ws);
    return 0;
}