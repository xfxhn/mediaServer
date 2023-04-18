


add_library(threadPool threadPool.cpp task.cpp rtspTask.cpp httpTask.cpp)

target_include_directories(threadPool
        PRIVATE ${PROJECT_SOURCE_DIR}/src
        )
target_link_libraries(threadPool
        PRIVATE socket
        PUBLIC protocol
        )
