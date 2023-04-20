




add_subdirectory(log)
add_subdirectory(bitStream)
add_subdirectory(nalu)
add_subdirectory(adts)
add_subdirectory(AVPacket)
add_subdirectory(server)
add_subdirectory(socket)
add_subdirectory(threadPool)
add_subdirectory(protocol)
add_subdirectory(utils)
add_subdirectory(transportStream)
add_subdirectory(flashVideo)


add_executable(mediaServer main.cpp)
target_include_directories(mediaServer
        PRIVATE ${PROJECT_SOURCE_DIR}/src
        )

target_link_libraries(mediaServer PRIVATE server)

