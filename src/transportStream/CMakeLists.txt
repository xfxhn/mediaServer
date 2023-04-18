




add_library(transportStream OBJECT transportPacket.cpp demuxPacket.cpp SI.cpp PES.cpp)


#[[
target_include_directories(transportStream
        PUBLIC ${PROJECT_SOURCE_DIR}/src/bitStream
        PRIVATE ${PROJECT_SOURCE_DIR}/src/nalu
        PRIVATE ${PROJECT_SOURCE_DIR}/src/adts
        PRIVATE ${PROJECT_SOURCE_DIR}/src/utils
        PRIVATE ${PROJECT_SOURCE_DIR}/src/aaaa
        )]]
target_include_directories(transportStream
        PRIVATE ${PROJECT_SOURCE_DIR}/src
        )
