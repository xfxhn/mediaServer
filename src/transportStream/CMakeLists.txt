




add_library(transportStream OBJECT transportPacket.cpp demuxPacket.cpp SI.cpp PES.cpp)


target_include_directories(transportStream
        PRIVATE ${PROJECT_SOURCE_DIR}/src
        )
target_link_libraries(transportStream PRIVATE bitStream log nalu adts)
