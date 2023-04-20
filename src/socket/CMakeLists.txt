


add_library(socket TcpSocket.cpp UdpSocket.cpp)
target_include_directories(socket PRIVATE ${PROJECT_SOURCE_DIR}/src)

target_link_libraries(socket PRIVATE $<$<BOOL:${WIN32}>:ws2_32.lib> log)


