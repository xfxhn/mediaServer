




add_subdirectory(bitStream)
add_subdirectory(nalu)
add_subdirectory(adts)
add_subdirectory(server)
add_subdirectory(socket)
add_subdirectory(threadPool)
add_subdirectory(protocol)
add_subdirectory(utils)
add_subdirectory(transportStream)


add_executable(mediaServer test.cpp)
target_include_directories(mediaServer
        PRIVATE ${PROJECT_SOURCE_DIR}/src/server
        PRIVATE ${PROJECT_SOURCE_DIR}/src/nalu
        )

target_link_libraries(mediaServer PRIVATE server)