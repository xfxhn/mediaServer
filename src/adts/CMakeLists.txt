


add_library(adts adtsReader.cpp adtsHeader.cpp)


target_include_directories(adts
        PRIVATE ${PROJECT_SOURCE_DIR}/src/bitStream
        PRIVATE ${PROJECT_SOURCE_DIR}/src/utils
        PRIVATE ${PROJECT_SOURCE_DIR}/src/transportStream
        )