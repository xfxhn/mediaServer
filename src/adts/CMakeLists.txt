


add_library(adts adtsReader.cpp adtsHeader.cpp)


target_include_directories(adts
        PRIVATE ${PROJECT_SOURCE_DIR}/src
        )
target_link_libraries(adts PRIVATE bitStream utils)