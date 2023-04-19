

#include "transportPacket.h"
#include "log/logger.h"



#define TRANSPORT_STREAM_PACKETS_SIZE 188
/*SI PID分配*/
enum {
    PAT = 0x00, //节目关联表
    CAT = 0x01, //条件访问表
    TSDT = 0x02,//传输流描述表
    NIT_ST = 0x10,
    SDT_BAT_ST = 0x11,
    EIT_ST = 0x12,
    RST_ST = 0x13,
    TDT_TOT_ST = 0x14
};

static uint8_t fillByte = 0xFF;


int TransportPacket::init() {
    buffer = new uint8_t[TRANSPORT_STREAM_PACKETS_SIZE];
    ws = new WriteStream(buffer, TRANSPORT_STREAM_PACKETS_SIZE);
    return 0;
}

void TransportPacket::resetPacketSize() {
    videoPacketSize = 0;
    audioPacketSize = 0;
}


int TransportPacket::transport_packet() const {
    ws->writeMultiBit(8, sync_byte);
    ws->writeMultiBit(1, transport_error_indicator);
    ws->writeMultiBit(1, payload_unit_start_indicator);
    ws->writeMultiBit(1, transport_priority);
    ws->writeMultiBit(13, PID);
    ws->writeMultiBit(2, transport_scrambling_control);
    ws->writeMultiBit(2, adaptation_field_control);
    ws->writeMultiBit(4, continuity_counter);

    /*
     * adaptation_field_control=0,是保留以后使用
     * adaptation_field_control=1，没有adaptation_field，只有有效载荷
     * adaptation_field_control=2，只有adaptation_field，没有有效载荷
     * adaptation_field_control=3，adaptation_field后面跟着有有效载荷
     * */
    /*if (adaptation_field_control == 2 || adaptation_field_control == 3) {
        ret = adaptation_field(rs);
        if (ret < 0) {
            fprintf(stderr, "解析adaptation_field失败\n");
            return ret;
        }
    }*/

    if (adaptation_field_control == 3) {
        adaptation_field();
    }


    /*
     一个 PES 包经过 TS 复用器会拆分成多个定长的 TS 包，那么怎么知道 PES 包从哪个 TS 包开始呢？
     payload_unit_start_indicator 的作用就在此，当 TS 包有效载荷包含 PES 包数据时，payload_unit_start_indicator 具有以下意义：
     1 指示此 TS 包的有效载荷随着 PES 包的首字节开始，
     0 指示在此 TS 包中无任何 PES 包将开始。

     当 TS 包有效载荷包含 PSI 数据时，payload_unit_start_indicator 具有以下意义：
     若 TS 包承载 PSI 分段的首字节，则 payload_unit_start_indicator 值必为 1，
     指示此 TS 包的有效载荷的首字节承载 pointer_field。
     若 TS 包不承载 PSI 分段的首字节，则 payload_unit_start_indicator 值为 0，
     指示在此有效载荷中不存在 pointer_field
*/
/*
	如果是PES包的话，payload_unit_start_indicator有两个值，0或1，具体的意思我们来举个例子。
	假设有两个视频帧，每个视频帧假设都需要3个ts packet包来存放一个PES包。那么一共有6个ts packet，
	它们的payload_unit_start_indicator的值为1,0,0,1,0,0，值为1代表一个帧的开始，下一个1就是新的一帧的开始了
*/
/*

 与 PES 包传输一样，通过 TS 包传送 PSI 表时，因为 TS 包的数据负载能力是有限的，
 即每个 TS 包的长度有限，所有当 PSI 表比较大时，PSI 被分成多段（section），再由多个 TS 包传输段。
 每一个段的长度不一，一个段的开始由 TS 包的有效负载中的 payload_unit_start_indicator 来标识
*/


    return 0;
}

int TransportPacket::writeTable(std::ofstream &fs) {
    writeServiceDescriptionTable();
    fs.write(reinterpret_cast<const char *>(buffer), TRANSPORT_STREAM_PACKETS_SIZE);

    writeProgramAssociationTable();
    fs.write(reinterpret_cast<const char *>(buffer), TRANSPORT_STREAM_PACKETS_SIZE);

    writeProgramMapTable();
    fs.write(reinterpret_cast<const char *>(buffer), TRANSPORT_STREAM_PACKETS_SIZE);
    return 0;
}

int TransportPacket::writeServiceDescriptionTable() {
    if (!ws) {
        fprintf(stderr, "请初始化\n");
        return -1;
    }
    ws->reset();
    setTransportPacketConfig(1, SDT_BAT_ST, 1, 0);
    transport_packet();
    ws->writeMultiBit(8, pointer_field);
    info.service_description_section(ws);
    ws->fillByte(0xFF);
    return 0;
}

int TransportPacket::writeProgramAssociationTable() {
    /*不够188字节，尾部填充0xFF，参考Table 2-31 – table_id assignment values*/
    if (!ws) {
        fprintf(stderr, "请初始化\n");
        return -1;
    }
    ws->reset();
    setTransportPacketConfig(1, PAT, 1, 0);
    transport_packet();
    ws->writeMultiBit(8, pointer_field);
    info.program_association_section(ws);
    ws->fillByte(0xFF);
    return 0;
}

int TransportPacket::writeProgramMapTable() {
    if (!ws) {
        fprintf(stderr, "请初始化\n");
        return -1;
    }
    ws->reset();
    setTransportPacketConfig(1, info.pat.program_map_PID, 1, 0);
    transport_packet();
    ws->writeMultiBit(8, pointer_field);
    info.program_map_section(ws);
    ws->fillByte(0xFF);
    return 0;
}

/*	访问单元分隔符*/
static constexpr uint8_t accessUnitSeparator[6] = {0x00, 0x00, 0x00, 0x01, 0x09, 0xf0};


int TransportPacket::writeVideoFrame(const NALPicture *picture, std::ofstream &fs) {
    if (!ws) {
        log_error("请初始化");
        return -1;
    }
    ws->reset();
    /*总共还有多少字节*/
    uint32_t totalByteSize = picture->size;
    /*还有多少空闲字节*/
    uint32_t unoccupiedByte = 0;
    /*这个参数读取到这个nalu第几个字节*/
    uint32_t offset = 0;
    /*当前读取到第几个nal*/
    int i = 0;
    uint32_t iSize = picture->data[i].nalUintSize;

    time = picture->pcr;
    /*写入ts头*/
    setTransportPacketConfig(1, info.pmt.videoPid, 3, videoPacketSize++);
    /*19是pes头的大小，5是ts头4字节加上Length 1字节，7是field那些字段占据的字节数,6是固定的访问单元分隔符*/
    /*totalByteSize <= 188 - 5 - 19 - 7 - 6*/
    if (totalByteSize <= 188 - 5 - 19 - 7) {
        /*需要填充多少字节*/
        uint16_t adaptationFieldLength = 188 - 5 - 19 - totalByteSize;
        setAdaptationFieldConfig(picture->sliceHeader.nalu.IdrPicFlag, adaptationFieldLength, true);
        transport_packet();
        ws->paddingByte(adaptationFieldLength - 7, 0xFF);
    } else {
        setAdaptationFieldConfig(picture->sliceHeader.nalu.IdrPicFlag, 7, true);
        transport_packet();
    }
    /*写入pes头*/
    pes.set_PTS_DTS_flags(3, picture->pts, picture->dts);
    pes.PES_packet(ws);


    /*先写入ts层和pes头*/
    fs.write(reinterpret_cast<const char *>(ws->bufferStart), ws->position);
    /*transportStreamFileSystem.write(reinterpret_cast<const char *>(accessUnitSeparator), 6);
    ws->setBytePtr(6);*/
    /*还有多少空闲字节*/
    unoccupiedByte = ws->bufferSize - ws->position;
    /*这个循环只是把188里剩余的字节给填满*/
    while (unoccupiedByte > 0) {
        // 剩余字节小于等于当前这个size
        if (unoccupiedByte < iSize) {
            fs.write(reinterpret_cast<const char *>(picture->data[i].data + offset),
                     unoccupiedByte);

            offset += unoccupiedByte;
            iSize -= unoccupiedByte;
            totalByteSize -= unoccupiedByte;
            unoccupiedByte = 0;
        } else {
            /*空闲字节大于等于这个nalu的大小*/
            fs.write(reinterpret_cast<const char *>(picture->data[i].data + offset), iSize);

            unoccupiedByte -= iSize;
            totalByteSize -= iSize;
            offset = 0;
            if (totalByteSize != 0)
                iSize = picture->data[++i].nalUintSize;

        }
    }

    while (totalByteSize >= 188 - 4) {

        /*重置一下，避免重复申请释放空间，浪费性能*/
        ws->reset();


        setTransportPacketConfig(0, info.pmt.videoPid, 1, videoPacketSize++);
        setAdaptationFieldConfig(false, 0, false);
        transport_packet();

        /*先写入ts层和pes头*/
        fs.write(reinterpret_cast<const char *>(ws->bufferStart), ws->position);
        /*还有多少空闲字节*/
        unoccupiedByte = ws->bufferSize - ws->position;

        /*这个循环只是把188里剩余的字节给填满*/
        while (unoccupiedByte > 0) {
            if (unoccupiedByte < iSize) {
                fs.write(reinterpret_cast<const char *>(picture->data[i].data + offset),
                         unoccupiedByte);

                offset += unoccupiedByte;
                iSize -= unoccupiedByte;
                totalByteSize -= unoccupiedByte;
                unoccupiedByte = 0;
            } else {
                /*空闲字节大于等于这个nalu的大小*/
                fs.write(reinterpret_cast<const char *>(picture->data[i].data + offset), iSize);

                unoccupiedByte -= iSize;
                totalByteSize -= iSize;
                offset = 0;
                if (totalByteSize != 0)
                    iSize = picture->data[++i].nalUintSize;
            }
        }

    }

    /*写入剩余的数据*/
    if (totalByteSize > 0) {
        ws->reset();
        /*5是ts层4字节和 adaptation_field_length 1字节，固定5字节,1是我定义的私有字段，表示这个ts包是这帧结束*/
        uint16_t adaptationFieldLength = 188 - 5 - totalByteSize;
        setTransportPacketConfig(0, info.pmt.videoPid, 3, videoPacketSize++);
        setAdaptationFieldConfig(false, adaptationFieldLength, false);
        transport_packet();

        fs.write(reinterpret_cast<const char *>(ws->bufferStart), ws->position);
        /*这里从1开始，因为跟在adaptation_field_length后面的那些参数有一字节*/
        for (int j = 1; j < adaptationFieldLength; ++j) {
            fs.write(reinterpret_cast<const char *>(&fillByte), 1);
        }

        fs.write(reinterpret_cast<const char *>(picture->data[i].data + offset), iSize);
        for (int j = i + 1; j < picture->data.size(); ++j) {
            fs.write(reinterpret_cast<const char *>(picture->data[j].data),
                     picture->data[j].nalUintSize);
        }

    }

    return 0;
}

int TransportPacket::writeAudioFrame(const AdtsHeader &header, std::ofstream &fs) {
    if (!ws) {
        log_error("请初始化");
        return -1;
    }
    ws->reset();

    /*总共还有多少字节*/
    uint32_t totalByteSize = header.size;
    uint8_t *data = header.data;
    /*还有多少空闲字节*/
    uint32_t unoccupiedByte = 0;
    uint32_t offset = 0;

    /*写入ts头*/
    setTransportPacketConfig(1, info.pmt.audioPid, 3, audioPacketSize++);
    if (totalByteSize <= 188 - 5 - 14 - 1) {
        /*需要填充多少字节  如果是 10 大小需要填充 159 字节*/
        uint16_t adaptationFieldLength = 188 - 5 - 14 - totalByteSize;
        setAdaptationFieldConfig(1, adaptationFieldLength, false);
        transport_packet();
        /*for (int i = 0; i < adaptationFieldLength - 1; ++i) {
            ws->writeMultiBit(8, 0xFF);
        }*/
        ws->paddingByte(adaptationFieldLength - 1, 0xFF);
    } else {
        setAdaptationFieldConfig(1, 1, false);
        transport_packet();
    }
    /*写入pes头*/
    pes.set_PTS_DTS_flags(2, header.pts, header.pts);
    pes.PES_packet(ws);

    fs.write(reinterpret_cast<const char *>(ws->bufferStart), ws->position);

    /*还有多少空闲字节*/
    unoccupiedByte = ws->bufferSize - ws->position;
    fs.write(reinterpret_cast<const char *>(data + offset), unoccupiedByte);
    totalByteSize -= unoccupiedByte;
    offset += unoccupiedByte;



    /*这里封装的是不是pes头的ts包*/
    while (totalByteSize >= 188 - 4) {
        /*重置一下，避免重复申请释放空间，浪费性能*/
        ws->reset();

        setTransportPacketConfig(0, info.pmt.audioPid, 1, audioPacketSize++);
        setAdaptationFieldConfig(0, 0, false);
        transport_packet();

        /*先写入ts层和pes头*/
        fs.write(reinterpret_cast<const char *>(ws->bufferStart), ws->position);
        /*还有多少空闲字节*/
        unoccupiedByte = ws->bufferSize - ws->position;


        fs.write(reinterpret_cast<const char *>(data + offset), unoccupiedByte);
        offset += unoccupiedByte;
        totalByteSize -= unoccupiedByte;

    }

    /*写入剩余的数据*/
    if (totalByteSize > 0) {
        ws->reset();
        /*5是ts层4字节和 adaptation_field_length 1字节，固定5字节*/
        uint16_t adaptationFieldLength = 188 - 5 - totalByteSize;
        setTransportPacketConfig(0, info.pmt.audioPid, 3, audioPacketSize++);
        setAdaptationFieldConfig(1, adaptationFieldLength, false);
        transport_packet();

        fs.write(reinterpret_cast<const char *>(ws->bufferStart), ws->position);
        /*这里从1开始，因为跟在adaptation_field_length后面的那些参数有一字节*/
        for (int j = 1; j < adaptationFieldLength; ++j) {
            fs.write(reinterpret_cast<const char *>(&fillByte), 1);
        }

        fs.write(reinterpret_cast<const char *>(data + offset), totalByteSize);
    }
    return 0;
}

int TransportPacket::adaptation_field() const {


    /*指定紧接在adaptation_field_length后面的adaptation_field的字节数*/
    ws->writeMultiBit(8, adaptation_field_length);

    if (adaptation_field_length > 0) {
        /*如果当前TS包相对于连续性计数器或程序时钟引用处于不连续状态，则设置为true*/
        ws->writeMultiBit(1, discontinuity_indicator);
        /*如果从此时开始可以正确解码流，则设置为true。
         * 比如这帧是idr帧，设置为true
         * 如果当前是p帧，b帧什么的，不能从当前这个pes开始解码，设置为false
         * */
        ws->writeMultiBit(1, random_access_indicator);
        ws->writeMultiBit(1, elementary_stream_priority_indicator);
        ws->writeMultiBit(1, PCR_flag);
        ws->writeMultiBit(1, OPCR_flag);
        ws->writeMultiBit(1, splicing_point_flag);
        ws->writeMultiBit(1, transport_private_data_flag);
        ws->writeMultiBit(1, adaptation_field_extension_flag);

        if (PCR_flag) {
            /*计算出毫秒*/
            uint64_t temp = time + 700;

            uint64_t pcr = temp * 27000;
            uint64_t pcr_high = pcr / 300;
            uint64_t pcr_low = pcr % 300;
            ws->writeMultiBit(33, pcr_high);
            ws->writeMultiBit(6, 0x3F);
            ws->writeMultiBit(9, pcr_low);
        }
    }


    return 0;
}

int TransportPacket::setTransportPacketConfig(uint8_t payloadUnitStartIndicator, uint16_t pid, uint8_t control,
                                              uint32_t counter) {
    transport_error_indicator = 0;
    payload_unit_start_indicator = payloadUnitStartIndicator;
    transport_priority = 0;
    PID = pid;
    transport_scrambling_control = 0;
    adaptation_field_control = control;
    continuity_counter = counter % 16;
    return 0;
}

int
TransportPacket::setAdaptationFieldConfig(uint8_t randomAccessIndicator, uint16_t adaptationFieldLength, bool flag) {


    adaptation_field_length = adaptationFieldLength;
    random_access_indicator = randomAccessIndicator;

    PCR_flag = flag;
    return 0;
}

TransportPacket::~TransportPacket() {
    delete[] buffer;
    delete ws;
}
















