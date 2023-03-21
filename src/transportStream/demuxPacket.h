

#ifndef RTSP_DEMUXPACKET_H
#define RTSP_DEMUXPACKET_H

#include <cstdint>
#include "readStream.h"

#define VIDEO_PID 256
#define AUDIO_PID 257

class DemuxPacket {

private:
    uint8_t sync_byte{0x47};
    uint8_t transport_error_indicator{0};
    uint8_t payload_unit_start_indicator{0};
    uint8_t transport_priority{0};
    uint16_t PID{0};
    uint8_t transport_scrambling_control{0};
    uint8_t adaptation_field_control{0};
    uint8_t continuity_counter{0};
    uint8_t pointer_field{0};

    /*adaptation_field*/
    uint8_t adaptation_field_length{1};
    uint8_t discontinuity_indicator{0};
    uint8_t random_access_indicator{0};
    uint8_t elementary_stream_priority_indicator{0};
    uint8_t PCR_flag{0};
    uint8_t OPCR_flag{0};
    uint8_t splicing_point_flag{0};
    uint8_t transport_private_data_flag{0};
    uint8_t adaptation_field_extension_flag{0};
    uint64_t program_clock_reference_base{0};
    uint16_t program_clock_reference_extension{0};
    uint64_t original_program_clock_reference_base{0};
    uint16_t original_program_clock_reference_extension{0};
    uint8_t splice_countdown{0};
    uint8_t transport_private_data_length{0};
    uint8_t *private_data_byte{nullptr};
    uint8_t adaptation_field_extension_length{0};
    uint8_t ltw_flag{0};
    uint8_t piecewise_rate_flag{0};
    uint8_t seamless_splice_flag{0};
    uint8_t ltw_valid_flag{0};
    uint16_t ltw_offset{0};
    uint32_t piecewise_rate{0};
    uint8_t splice_type{0};
public:

    int readVideoFrame(ReadStream &rs);

private:
    int adaptation_field(ReadStream &rs);
};


#endif //RTSP_DEMUXPACKET_H
