
#include "http.h"

//#include <sstream>
#include <fstream>
#include <filesystem>
#include <cstring>
#include "util.h"


char Http::response[1024]{0};

int Http::init(SOCKET socket) {
    clientSocket = socket;
    return 0;
}


int Http::parse(std::string &packet, const std::string &data) {
    int ret;


    std::string method, path, version;

    std::vector<std::string> list = split(data, "\r\n");

    std::istringstream iss(list.front());
    iss >> method >> path >> version;

    list.erase(list.begin());
    request = getObj(list, ":");

    if (method != "GET") {
        fprintf(stderr, "method = %s\n", method.c_str());
        responseData(405, "Method Not Allowed");
        return -1;
    }


    std::filesystem::path source("." + path);

    if (!std::filesystem::exists(source)) {
        responseData(404, "找不到对应的流");
        return -1;
    }

    std::string mimeType;
    if (source.extension() == ".m3u8") {
        mimeType = "application/x-mpegurl";
        ret = disposeTransStream(source.string(), mimeType);
        if (ret < 0) {
            fprintf(stderr, "disposeTransStream 失败\n");
            return ret;
        }
    } else if (source.extension() == ".ts") {
        mimeType = "video/mp2t";
        ret = disposeTransStream(source.string(), mimeType);
        if (ret < 0) {
            fprintf(stderr, "disposeTransStream 失败\n");
            return ret;
        }
    } else if (source.extension() == ".flv") {
        mimeType = "video/x-flv";
        ret = disposeFLV(source.string(), mimeType);
        if (ret < 0) {
            fprintf(stderr, "disposeTransStream 失败\n");
            return ret;
        }
    } else {
        fprintf(stderr, "不支持这种类型\n");
        responseData(415, "Unsupported Media Type");
        return -1;
    }


    return 0;
}


int Http::disposeFLV(const std::string& path, const std::string& mimeType) {
    return 0;
}


int Http::disposeTransStream(const std::string& path, const std::string& mimeType) {
    int ret;
    std::ifstream fs(path, std::ios::binary | std::ios::ate);
    if (!fs.good()) {
        fprintf(stderr, "当前文件不可读 %s\n", ("." + path).c_str());
        responseData(500, "当前文件不可读");
        return -1;
    }

    uint32_t size = fs.tellg();
    fs.seekg(0, std::ios::beg);


    std::ostringstream oss;
    oss << "HTTP/1.1 200 OK\r\n";
    oss << "Content-Length: " << size << "\r\n";
    oss << "Content-Type: " << mimeType << "\r\n";
    oss << "\r\n";
    oss << fs.rdbuf();

    std::string res = oss.str();
    ret = TcpSocket::sendData(clientSocket, reinterpret_cast<uint8_t *>(const_cast<char *>(res.c_str())),
                              static_cast<int>(res.size()));
    if (ret < 0) {
        fprintf(stderr, "发送数据失败\n");
        return ret;
    }
    return 0;
}


int Http::responseData(int status, const std::string &msg) const {
    memset(response, 0, 1024);
    std::string body = "<html><body><h1>" + msg + "</h1></body></html>";
    sprintf(response,
            "HTTP/1.1 %d %s\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: %zu\r\n"
            "\r\n"
            "%s",
            status, msg.c_str(),
            body.size(),
            body.c_str()
    );

    int ret = TcpSocket::sendData(clientSocket, reinterpret_cast<uint8_t *>(response),
                                  static_cast<int>(strlen(response)));
    if (ret < 0) {
        fprintf(stderr, "发送数据失败\n");
        return ret;
    }
    return 0;
}

Http::~Http() = default;
