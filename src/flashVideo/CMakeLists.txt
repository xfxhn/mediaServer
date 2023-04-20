


add_library(flashVideo FLVHeader.cpp FLVTagHeader.cpp FLVScriptTag.cpp FLVVideoTag.cpp FLVAudioTag.cpp)


target_include_directories(flashVideo
        PRIVATE ${PROJECT_SOURCE_DIR}/src
        )

target_link_libraries(flashVideo PRIVATE bitStream)