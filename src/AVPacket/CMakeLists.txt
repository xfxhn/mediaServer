



add_library(AVPacket AVPacket.cpp)

target_include_directories(AVPacket
        PRIVATE ${PROJECT_SOURCE_DIR}/src/nalu
        PRIVATE ${PROJECT_SOURCE_DIR}/src/transportStream
        PRIVATE ${PROJECT_SOURCE_DIR}/src/adts
        PRIVATE ${PROJECT_SOURCE_DIR}/src/bitStream
        )