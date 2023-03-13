


add_library(socket TcpSocket.cpp UdpSocket.cpp)

target_link_libraries(socket PRIVATE ws2_32.lib)