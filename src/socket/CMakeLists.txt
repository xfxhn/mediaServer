


add_library(socket TcpSocket.cpp UdpSocket.cpp)
target_include_directories(socket PRIVATE ${PROJECT_SOURCE_DIR}/src)
if (WIN32)
    target_link_libraries(socket PRIVATE ws2_32.lib)
endif ()

