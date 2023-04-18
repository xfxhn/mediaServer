



add_library(AVPacket AVPacket.cpp)


target_include_directories(AVPacket
        PRIVATE ${PROJECT_SOURCE_DIR}/src
        )

target_link_libraries(AVPacket PRIVATE transportStream)
