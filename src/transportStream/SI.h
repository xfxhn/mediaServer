

#ifndef TS_SI_H
#define TS_SI_H

#include <cstdint>

#define VIDEO_PID 256
#define AUDIO_PID 257

class WriteStream;

enum {
    PROGRAM_ASSOCIATION_SECTION = 0x00,
    conditional_access_section = 0x01,
    PROGRAM_MAP_SECTION = 0x02,
    /*实际的网络*/
    network_information_section = 0x40,
    /*其他网络*/
    /*network_information_section_other = 0x41,*/
    /*实际运输流*/
    SERVICE_DESCRIPTION_SECTION = 0x42,
    /*其他运输流*/
    /*service_description_section_other = 0x46,*/
    BOUQUET_ASSOCIATION_SECTION = 0x4A,

    /*后面还有一些table_id对应的一些表，具体参考SI规范 Table 2: Allocation of table_id values*/
};
struct SDT {
    uint8_t table_id{SERVICE_DESCRIPTION_SECTION};
    uint8_t section_syntax_indicator{1};
    /*它指定section的字节数，从紧接着section_length字段开始，包括CRC*/
    /*section_length不能超过1021，这样整个section的最大长度为1024字节。*/
    uint16_t section_length{39};
    uint16_t transport_stream_id{1};
    uint8_t version_number{0};
    uint8_t current_next_indicator{1};
    uint8_t section_number{0};
    uint8_t last_section_number{0};
    uint16_t original_network_id{65281};
    uint16_t service_id{1};
    uint8_t EIT_schedule_flag{0};
    uint8_t EIT_present_following_flag{0};
    uint8_t running_status{4};
    uint8_t free_CA_mode{0};
    uint8_t descriptors_loop_length{22};
    /*uint8_t CRC_32{0};*/


    uint8_t descriptor_tag{0x48};
    uint8_t descriptor_length{20};
    uint8_t service_type{0x01};
    uint8_t service_provider_name_length{8};
    uint8_t service_name_length{9};
};
struct PAT {
    uint8_t table_id{PROGRAM_ASSOCIATION_SECTION};
    uint8_t section_syntax_indicator{1};
    /*它指定section的字节数，从紧接着section_length字段开始，包括CRC*/
    /*section_length不能超过1021，这样整个section的最大长度为1024字节。*/
    uint16_t section_length{13};
    uint16_t transport_stream_id{1};
    uint8_t version_number{0};
    /*设置为1表示当前表可以用，为0当前表不可以用*/
    uint8_t current_next_indicator{1};
    uint8_t section_number{0};
    uint8_t last_section_number{0};

    uint16_t program_number{1};
    uint16_t program_map_PID{4096};

    /*uint32_t CRC_32{0};*/
};

struct PMT {
    uint8_t table_id{PROGRAM_MAP_SECTION};
    uint8_t section_syntax_indicator{1};
    uint16_t section_length{23};
    uint8_t version_number{0};
    uint8_t current_next_indicator{1};
    uint8_t section_number{0};
    uint8_t last_section_number{0};
    uint16_t PCR_PID{VIDEO_PID};/*0x1FFF*/

    uint16_t videoPid{VIDEO_PID};
    uint16_t audioPid{AUDIO_PID};
    /*uint32_t CRC_32{0};*/


};

class SI {
public:
    PAT pat;
    SDT sdt;
    PMT pmt;

public:
    SI();

    int program_association_section(WriteStream *ws) const;

    int program_map_section(WriteStream *ws) const;

    int service_description_section(WriteStream *ws);


private:


    int service_descriptor(WriteStream *ws) const;

    static uint32_t table[256];

    static void makeTable();

    static uint32_t crc32Calculate(uint8_t *buffer, uint32_t size);
};


#endif //TS_SI_H
