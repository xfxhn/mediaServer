

add_library(
        nalu NALReader.cpp NALHeader.cpp NALSeqParameterSet.cpp
        NALPictureParameterSet.cpp NALSliceHeader.cpp NALDecodedPictureBuffer.cpp
        NALPicture.cpp
)


target_include_directories(nalu
        PRIVATE ${PROJECT_SOURCE_DIR}/src/bitStream
        PRIVATE ${PROJECT_SOURCE_DIR}/src/transportStream
        PRIVATE ${PROJECT_SOURCE_DIR}/src/utils
        )


target_link_libraries(nalu
        PRIVATE transportStream
        )
