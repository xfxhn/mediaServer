

#include "AVReadPacket.h"
#include <thread>
#include <filesystem>

#include "bitStream/readStream.h"
#include "log/logger.h"

/*存储一个启动时间，然后在每个要发送的函数里获取当前时间，用当前的这个时间减去启动时间
 * 然后用这个时间和dts作比较，如果dts大于这个时间，就不发送
 * */

int AVReadPacket::init(const std::string &dir, uint32_t transportStreamPacketNumber) {
    path = dir;
    currentPacket = transportStreamPacketNumber;

    std::string name = "/test" + std::to_string(currentPacket) + ".ts";
    log_info("读取%s文件", name.c_str());

    fs.open(path + name, std::ios::out | std::ios::binary);
    if (!fs.is_open()) {
        log_error("open %s failed", (path + name).c_str());
        return -1;
    }


    videoReader.init();
    audioReader.init();


    picture = videoReader.allocPicture();
    start = std::chrono::high_resolution_clock::now();
    return 0;
}


int AVReadPacket::readTransportStream(bool videoFlag, bool audioFlag) {

    int ret = 0;

    elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start);
    /*需要视频的dts和音频的dts都小于这个时间才会去读取*/
    if (header.pts > elapsed.count() || picture->dts > elapsed.count()) {
        return 1;
    }

    std::string name;
    uint32_t size = 0;
    while (true) {

        fs.read(reinterpret_cast<char *>(transportStreamBuffer + size), TRANSPORT_STREAM_PACKETS_SIZE - size);
        size += fs.gcount();
        if (size == 0) {
            name = "/test" + std::to_string(++currentPacket) + ".ts";
            if (!std::filesystem::exists(path + name)) {
//                /*没有这个文件，说明这个文件还没读完，要继续读这个文件*/
//                --currentPacket;
//                fs.clear();
//                fprintf(stderr, "没有下个文件,等待一会儿\n");
//                std::this_thread::sleep_for(std::chrono::milliseconds(100));
//                continue;
                log_info("没有文件可以读了，退出");
                return AVERROR_EOF;
            }

            /*表示这个文件读完了，读下一个*/
            fs.close();
            fs.open(path + name, std::ios::out | std::ios::binary);
            if (!fs.is_open()) {
                fprintf(stderr, "读取%s失败 video\n", name.c_str());
                return -1;
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
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        /*走到这里肯定有188个字节*/
        ReadStream rs(transportStreamBuffer, size);
        ret = demux.readFrame(rs);
        if (ret < 0) {
            fprintf(stderr, "demux.readVideoFrame失败\n");
            return ret;
        }
        if (ret == VIDEO_PID && videoFlag) {
            size = rs.size - rs.position;
            videoReader.putData(rs.currentPtr, size);
            break;
        } else if (ret == AUDIO_PID && audioFlag) {
            size = rs.size - rs.position;
            audioReader.putData(rs.currentPtr, size);
            break;
        }
        size = 0;
    }


    return ret;
}


int AVReadPacket::getParameter() {
    int ret;

    bool videoFlag = true;
    bool audioFlag = true;
    while (videoFlag || audioFlag) {
        videoReader.resetBuffer();
        audioReader.resetBuffer();

        ret = readTransportStream(videoFlag, audioFlag);
        if (ret < 0) {
            fprintf(stderr, "getTransportStreamData 失败\n");
            return ret;
        }
        if (ret == VIDEO_PID && videoFlag) {
            ret = videoReader.getParameter();
            if (ret < 0) {
                fprintf(stderr, "videoReader.getParameter 失败\n");
                return -1;
            }

            if (ret == 3) {
                // sps
                sps = videoReader.sps;
                spsData = videoReader.spsData;
                spsSize = videoReader.spsSize;
            } else if (ret == 4) {
                // pps
                pps = videoReader.pps;
                ppsData = videoReader.ppsData;
                ppsSize = videoReader.ppsSize;
                videoFlag = false;
            }
        } else if (ret == AUDIO_PID && audioFlag) {
            ret = audioReader.getParameter();
            if (ret < 0) {
                fprintf(stderr, "audioReader.getParameter 失败\n");
                return -1;
            }
            if (ret == 1) {
                aacHeader = audioReader.parameter;
                audioFlag = false;
            }
        }
    }
    return 0;
}


int AVReadPacket::readFrame(AVPackage *package) {
    int ret;
    header.finishFlag = false;
    picture->finishFlag = false;


    videoReader.resetBuffer();
    audioReader.resetBuffer();

    while (true) {
        /*有可能这个时候读ts tag包的是video，但是这个ts tag包凑不够一帧，下个ts tag包是audio*/
        ret = readTransportStream();
        if (ret < 0) {
            if (ret == AVERROR_EOF) {
                return AVERROR_EOF;
            }
            log_error("getTransportStreamData 失败");
            return ret;
        }

        if (ret == 1) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }


        if (ret == VIDEO_PID) {
            ret = videoReader.getVideoFrame3(picture);
            if (ret < 0) {
                fprintf(stderr, "videoReader.getVideoFrame3 失败\n");
                return -1;
            }
            /*
             * 每次进来获取一帧数据的时候把finishFlag设置为false，然后循环去读取数据，
             * 只有读够了完整的一帧，finishFlag才会被设置为true
             * */
            if (picture->finishFlag) {
                package->idrFlag = picture->sliceHeader.nalu.IdrPicFlag;
                package->fps = picture->sliceHeader.sps.fps;
                package->pts = picture->pts;
                package->dts = picture->dts;
                package->decodeFrameNumber = picture->decodeFrameNumber;
                package->data1 = picture->data;
                package->size = picture->size;
                package->type = "video";
                break;
            }

        } else if (ret == AUDIO_PID) {
            ret = audioReader.getAudioFrame2(header);
            if (ret < 0) {
                fprintf(stderr, "audioReader.getAudioFrame2 失败\n");
                return -1;
            }

            if (header.finishFlag) {
                package->idrFlag = true;
                package->fps = header.sample_rate;
                package->dts = header.dts;
                package->pts = header.pts;
                package->data2 = header.data;
                package->size = header.size;
                package->type = "audio";
                break;
            }
        }

    }
    return 0;
}

AVPackage *AVReadPacket::allocPacket() {
    return new AVPackage;
}

void AVReadPacket::freePacket(AVPackage *package) {
    delete package;
}

AVReadPacket::~AVReadPacket() {
    fs.close();
}




