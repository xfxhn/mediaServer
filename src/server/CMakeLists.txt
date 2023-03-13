

#[[ AnalysisData.cpp rtpPacket.cpp]]
add_library(server server.cpp)

#[[PUBLIC ${PROJECT_SOURCE_DIR}/src/socket]]
target_include_directories(server
        PRIVATE ${PROJECT_SOURCE_DIR}/src/bitStream
        PRIVATE ${PROJECT_SOURCE_DIR}/src/configurationRecord
        PRIVATE ${PROJECT_SOURCE_DIR}/src/utils
        PUBLIC ${PROJECT_SOURCE_DIR}/src/threadPool
        )
#target_compile_definitions(mediaServer PRIVATE _CRT_SECURE_NO_WARNINGS)
#[[configurationRecord socket]]
target_link_libraries(server PUBLIC threadPool)


