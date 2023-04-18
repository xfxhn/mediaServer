

add_library(server server.cpp)

target_include_directories(server PRIVATE ${PROJECT_SOURCE_DIR}/src)
#target_compile_definitions(mediaServer PRIVATE _CRT_SECURE_NO_WARNINGS)
#[[configurationRecord socket]]
target_link_libraries(server PUBLIC log threadPool PRIVATE $<$<BOOL:${UNIX}>:pthread>)


