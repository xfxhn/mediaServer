
#include "util.h"
#include <bitset>
#include <functional>

static uint8_t alphabet_map[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static uint8_t reverse_map[] =
        {
                255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
                255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
                255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 62, 255, 255, 255, 63,
                52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 255, 255, 255, 255, 255, 255,
                255, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
                15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 255, 255, 255, 255, 255,
                255, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
                41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 255, 255, 255, 255, 255
        };

std::string trim(const std::string &str) {
    if (str.empty()) {
        return str;
    }

    size_t start = str.find_first_not_of(' ');
    size_t end = str.find_last_not_of(' ');
    return str.substr(start, end - start + 1);
}

int base64_encode(const std::string &text, uint8_t *encode) {
    uint32_t text_len = text.length();
    int i, j;
    for (i = 0, j = 0; i + 3 <= text_len; i += 3) {
        encode[j++] = alphabet_map[text[i] >> 2];                             //取出第一个字符的前6位并找出对应的结果字符
        encode[j++] = alphabet_map[((text[i] << 4) & 0x30) |
                                   (text[i + 1] >> 4)];     //将第一个字符的后2位与第二个字符的前4位进行组合并找到对应的结果字符
        encode[j++] = alphabet_map[((text[i + 1] << 2) & 0x3c) |
                                   (text[i + 2] >> 6)];   //将第二个字符的后4位与第三个字符的前2位组合并找出对应的结果字符
        encode[j++] = alphabet_map[text[i + 2] & 0x3f];                         //取出第三个字符的后6位并找出结果字符
    }

    if (i < text_len) {
        uint32_t tail = text_len - i;
        if (tail == 1) {
            encode[j++] = alphabet_map[text[i] >> 2];
            encode[j++] = alphabet_map[(text[i] << 4) & 0x30];
            encode[j++] = '=';
            encode[j++] = '=';
        } else //tail==2
        {
            encode[j++] = alphabet_map[text[i] >> 2];
            encode[j++] = alphabet_map[((text[i] << 4) & 0x30) | (text[i + 1] >> 4)];
            encode[j++] = alphabet_map[(text[i + 1] << 2) & 0x3c];
            encode[j++] = '=';
        }
    }
    return j;
}

int base64_decode(const std::string &code, uint8_t *plain) {
    uint32_t code_len = code.length();
    // assert((code_len & 0x03) == 0);  //如果它的条件返回错误，则终止程序执行。4的倍数。
    if ((code_len & 0x03) != 0) {
        return -1;
    }
    int i, j = 0;
    uint8_t quad[4];
    for (i = 0; i < code_len; i += 4) {
        for (uint32_t k = 0; k < 4; k++) {
            quad[k] = reverse_map[code[i + k]];//分组，每组四个分别依次转换为base64表内的十进制数
        }

        if (quad[0] >= 64 && quad[1] >= 64) {
            return -1;
        }
        // assert(quad[0] < 64 && quad[1] < 64);

        plain[j++] = (quad[0] << 2) | (quad[1] >> 4); //取出第一个字符对应base64表的十进制数的前6位与第二个字符对应base64表的十进制数的前2位进行组合

        if (quad[2] >= 64)
            break;
        else if (quad[3] >= 64) {
            plain[j++] = (quad[1] << 4) | (quad[2] >> 2); //取出第二个字符对应base64表的十进制数的后4位与第三个字符对应base64表的十进制数的前4位进行组合
            break;
        } else {
            plain[j++] = (quad[1] << 4) | (quad[2] >> 2);
            plain[j++] = (quad[2] << 6) | quad[3];//取出第三个字符对应base64表的十进制数的后2位与第4个字符进行组合
        }
    }
    return j;
}

/*这个初始化可能会抛出异常，所以clion会给警告，并且无法捕获*/
static const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string generatorDate() {
    // 使用time函数获取当前时间，并存储在now变量中
    time_t now = time(nullptr);

    // 使用gmtime函数将now转换为GMT时间，并返回一个tm结构的指针
    tm *gmtm = gmtime(&now);

    // 创建一个足够大的缓冲区
    char buffer[64];

    // 使用strftime函数将tm结构转换为字符串，并指定格式为Date: %a, %d %b %Y %H:%M:%S GMT
    strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", gmtm);

    // 输出结果
    return buffer;
}

int decodeBase64(std::string text, std::vector<uint8_t> &data) {
    // 定义一个存储解码后的字节的向量
    // std::vector<uint8_t> data;
    // 定义一个存储解码后的文本的字符串
    // std::string result;
    // 去掉text末尾的=号
    while (text.back() == '=') {
        text.pop_back();
    }
    // 遍历text中的每个字符
    for (size_t i = 0; i < text.size(); i += 4) {
        // 定义一个存储24位二进制数的字符串
        std::string bits;
        // 对每个字符进行解码，转换成6位二进制数，并拼接到bits中
        for (size_t j = 0; j < 4; j++) {
            // 在base64字符表中查找字符的索引
            size_t index = base64_chars.find(text[i + j]);
            // 如果找不到，返回空字符串
            if (index == std::string::npos) {
                return -1;
            }
            // 将索引转换成6位二进制数，并拼接到bits中
            bits += std::bitset<6>(index).to_string();
        }
        // 将bits按照8位一组分割成字节，并存储到data中
        for (size_t k = 0; k < 24; k += 8) {
            // 将8位二进制数转换成无符号字符
            uint8_t byte = std::bitset<8>(bits.substr(k, 8)).to_ulong();
            // 将无符号字符存储到data中
            data.push_back(byte);
        }
    }
    return 0;
//    // 将data中的字节转换成字符串，并拼接到result中
//    for (auto byte: data) {
//        result += byte;
//    }
//    // 返回result
//    return result;
}

std::string encodeBase64(const uint8_t *data, uint32_t size) {
    /*static const char *base64_chars =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz"
            "0123456789+/";*/

    std::string encoded;
    size_t i = 0;
    uint32_t triad = 0;

    while (i < size) {
        switch (i % 3) {
            case 0:
                triad = (data[i] << 16) + (data[i + 1] << 8) + data[i + 2];
                encoded.push_back(base64_chars[(triad >> 18) & 0x3F]);
                encoded.push_back(base64_chars[(triad >> 12) & 0x3F]);
                encoded.push_back(base64_chars[(triad >> 6) & 0x3F]);
                encoded.push_back(base64_chars[triad & 0x3F]);
                i += 3;
                break;
            case 1:
                triad = (data[i] << 16) + (data[i + 1] << 8);
                encoded.push_back(base64_chars[(triad >> 18) & 0x3F]);
                encoded.push_back(base64_chars[(triad >> 12) & 0x3F]);
                encoded.push_back(base64_chars[(triad >> 6) & 0x3F]);
                encoded.push_back('=');
                i += 2;
                break;
            case 2:
                triad = (data[i] << 16);
                encoded.push_back(base64_chars[(triad >> 18) & 0x3F]);
                encoded.push_back(base64_chars[(triad >> 12) & 0x3F]);
                encoded.push_back('=');
                encoded.push_back('=');
                i += 1;
                break;
        }
    }
    return encoded;
}

std::string GenerateSpropParameterSets(const uint8_t *sps, uint32_t spsSize, uint8_t *pps, uint32_t ppsSize) {
    std::stringstream ss;

    ss << encodeBase64(sps, spsSize);
    ss << ",";
    ss << encodeBase64(pps, ppsSize);

    return ss.str();
}

std::string decimalToHex(int decimalNum) {
    std::stringstream ss;
    ss << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << decimalNum;
    return ss.str();
}

std::vector<std::string> split(const std::string &str, const std::string &spacer) {
    std::vector<std::string> list;
    using STRING_SIZE = std::string::size_type;
    STRING_SIZE pos1, pos2;
    STRING_SIZE len = spacer.length();     //记录分隔符的长度
    pos1 = 0;
    pos2 = str.find(spacer);
    while (pos2 != std::string::npos) {
        list.push_back(trim(str.substr(pos1, pos2 - pos1)));
        pos1 = pos2 + len;
        pos2 = str.find(spacer, pos1);    // 从str的pos1位置开始搜寻spacer
    }
    if (pos1 != str.length()) //分割最后一个部分
        list.push_back(str.substr(pos1));
    return list;
}

uint64_t av_rescale_q(uint64_t a, const AVRational &bq, const AVRational &cq) {
    //(1 / 25) / (1 / 1000);
    int64_t b = bq.num * cq.den;
    int64_t c = cq.num * bq.den;
    return a * b / c;  //25 * (1000 / 25)  把1000分成25份，然后当前占1000的多少
}


std::map<std::string, std::string> getObj(const std::vector<std::string> &list, const std::string &spacer) {
    std::map<std::string, std::string> obj;
    for (const std::string &str: list) {
        std::string::size_type pos = str.find(spacer);
        if (pos != std::string::npos) {
            std::string key = trim(str.substr(0, pos));
            std::string value = trim(str.substr(pos + 1));
            obj[key] = value;
        }
    }

    return obj;
}

std::string generate_unique_string() {
    static int counter = 0;
    return "xiaofeng_" + std::to_string(counter++);
}