

add_library(
        nalu NALReader.cpp NALHeader.cpp NALSeqParameterSet.cpp
        NALPictureParameterSet.cpp NALSliceHeader.cpp NALDecodedPictureBuffer.cpp
        NALPicture.cpp
)

target_include_directories(nalu
        PRIVATE ${PROJECT_SOURCE_DIR}/src
        )
target_link_libraries(nalu PRIVATE bitStream log)
