

#ifndef MUX_NALDECODEDPICTUREBUFFER_H
#define MUX_NALDECODEDPICTUREBUFFER_H

#include <cstdint>
#include <list>


#include "NALPicture.h"

class NALSliceHeader;

class NALSeqParameterSet;


class NALDecodedPictureBuffer {
public:
    NALPicture *dpb[16]{nullptr};
public:
    NALDecodedPictureBuffer();

    void reset();

    int decoding_finish(NALPicture *picture, NALPicture *&unoccupiedPicture);

    int decoding_process_for_reference_picture_lists_construction(NALPicture *picture);

    ~NALDecodedPictureBuffer();

private:
    int decoded_reference_picture_marking_process(NALPicture *picture);

    int sliding_window_decoded_reference_picture_marking_process(const NALPicture *picture);

    int adaptive_memory_control_decoded_reference_picture_marking_process(NALPicture *picture);


    int decoding_process_for_picture_numbers(NALPicture *picture);

    NALPicture *getUnoccupiedPicture(const NALPicture *picture);
};


#endif //MUX_NALDECODEDPICTUREBUFFER_H
