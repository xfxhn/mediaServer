



add_library(AVPacket AVReadPacket.cpp AVWritePacket.cpp)


target_include_directories(AVPacket
        PRIVATE ${PROJECT_SOURCE_DIR}/src
        )

target_link_libraries(AVPacket
        PRIVATE bitStream log nalu adts transportStream
        )
