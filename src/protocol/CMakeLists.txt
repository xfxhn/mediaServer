


add_library(protocol rtsp.cpp rtspSendData.cpp rtspReceiveData.cpp http.cpp httpHls.cpp httpFlv.cpp)


target_include_directories(protocol
        PRIVATE ${PROJECT_SOURCE_DIR}/src
        )
#[[todo 更改每个lib的依赖]]
target_link_libraries(protocol
        PRIVATE bitStream log utils socket flashVideo AVPacket
        )