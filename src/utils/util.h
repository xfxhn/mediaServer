
#ifndef RTSP_UTIL_H
#define RTSP_UTIL_H

#include <string>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <vector>
#include <map>

struct AVRational {
    int num;
    int den;
};

std::map<std::string, std::string> getObj(const std::vector<std::string> &list, const std::string &spacer);

uint64_t av_rescale_q(uint64_t a, const AVRational &bq, const AVRational &cq);

std::string trim(const std::string &str);

int base64_encode(const std::string &text, uint8_t *encode);

int base64_decode(const std::string &code, uint8_t *plain);

std::string generatorDate();

std::string encodeBase64(const uint8_t *data, uint32_t size);

int decodeBase64(std::string text, std::vector<uint8_t> &data);

std::string GenerateSpropParameterSets(const uint8_t *sps, uint32_t spsSize, uint8_t *pps, uint32_t ppsSize);

std::string decimalToHex(int decimalNum);

std::vector<std::string> split(const std::string &str, const std::string &spacer);


#endif //RTSP_UTIL_H
