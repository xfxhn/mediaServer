
add_library(log logger.cpp)

if (UNIX)
    target_compile_definitions(log PRIVATE LOG_USE_COLOR)
endif ()