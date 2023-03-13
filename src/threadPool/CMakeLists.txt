


add_library(threadPool threadPool.cpp task.cpp rtspTask.cpp)
target_include_directories(threadPool
        PUBLIC ${PROJECT_SOURCE_DIR}/src/socket
        PUBLIC ${PROJECT_SOURCE_DIR}/src/protocol
        )
target_link_libraries(threadPool
        PRIVATE socket
        PUBLIC protocol
        )
