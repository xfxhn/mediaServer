


add_library(socket TcpSocket.cpp UdpSocket.cpp)
if (WIN32)
    target_link_libraries(socket PRIVATE ws2_32.lib)
endif ()