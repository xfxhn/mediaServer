

add_library(server server.cpp)

target_include_directories(server PRIVATE ${PROJECT_SOURCE_DIR}/src)

target_link_libraries(server PRIVATE log socket threadPool PRIVATE $<$<BOOL:${UNIX}>:pthread>)


