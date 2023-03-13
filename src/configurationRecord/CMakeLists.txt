

add_library(configurationRecord audioSpecificConfig.cpp AVCDecoderConfigurationRecord.cpp)

target_include_directories(configurationRecord
        PRIVATE ${PROJECT_SOURCE_DIR}/src/bitStream
        PRIVATE ${PROJECT_SOURCE_DIR}/src/nalu
        )