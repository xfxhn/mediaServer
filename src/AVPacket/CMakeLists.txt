



add_library(AVPacket AVPacket.cpp)

target_include_directories(AVPacket
        PUBLIC ${PROJECT_SOURCE_DIR}/src/nalu
        PUBLIC ${PROJECT_SOURCE_DIR}/src/transportStream
        PUBLIC ${PROJECT_SOURCE_DIR}/src/adts
        PUBLIC ${PROJECT_SOURCE_DIR}/src/bitStream
        PUBLIC ${PROJECT_SOURCE_DIR}/src/utils
        )