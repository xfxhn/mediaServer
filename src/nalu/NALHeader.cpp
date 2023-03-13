#include "NALHeader.h"

/*header*/
void NALHeader::nal_unit(uint8_t header) {
    forbidden_zero_bit = header >> 7 & 1;
    nal_ref_idc = header >> 5 & 3;
    nal_unit_type = header & 31;
    IdrPicFlag = nal_unit_type == 5;

    /*
     不解析
    nalUnitHeaderBytes = 1
    if(nal_unit_type = = 14 || nal_unit_type = = 20 || nal_unit_type = = 21) {
        if( nal_unit_type ! = 21 )
            分别对应同一码流内可以包含具有不同帧率、不同分辨率、不同码率的分层编码方式
            svc_extension_flag All u(1)
        else
            表示3D编码
            avc_3d_extension_flag All u(1)

        if( svc_extension_flag ) {
            nal_unit_header_svc_extension() *//* specified in Annex G *//* All
            nalUnitHeaderBytes += 3
        } else if( avc_3d_extension_flag ) {
            nal_unit_header_3davc_extension() *//* specified in Annex J *//*
            nalUnitHeaderBytes += 2
        } else {
            nal_unit_header_mvc_extension() *//* specified in Annex H *//* All
            nalUnitHeaderBytes += 3
        }
    }*/
}

/*body*/
void NALHeader::ebsp_to_rbsp(uint8_t *data, uint32_t &size) {
    uint32_t NumBytesInRBSP = 0;
    for (uint32_t i = 1; i < size; i++) {
        if ((i + 2 < size) && data[i] == 0 && data[i + 1] == 0 && data[i + 2] == 3) {
            data[NumBytesInRBSP++] = data[i];
            data[NumBytesInRBSP++] = data[i + 1];
            i += 2;
        } else {
            data[NumBytesInRBSP++] = data[i];
        }
    }
    size = NumBytesInRBSP;
}
