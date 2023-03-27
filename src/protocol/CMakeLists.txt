


add_library(protocol rtsp.cpp rtpPacket.cpp rtspReceiveData.cpp)

target_include_directories(protocol
        PUBLIC ${PROJECT_SOURCE_DIR}/src/nalu
        PUBLIC ${PROJECT_SOURCE_DIR}/src/adts
        PUBLIC ${PROJECT_SOURCE_DIR}/src/utils
        PRIVATE ${PROJECT_SOURCE_DIR}/src/socket
        PRIVATE ${PROJECT_SOURCE_DIR}/src/bitStream
        PUBLIC ${PROJECT_SOURCE_DIR}/src/transportStream
        )

target_link_libraries(protocol
        PRIVATE nalu
        PRIVATE adts
        PRIVATE utils
        PRIVATE bitStream
        PUBLIC transportStream
        )