
#include "http.h"

//#include <sstream>
#include <fstream>
#include <filesystem>
#include "util.h"

int Http::init(SOCKET socket) {
    clientSocket = socket;
    return 0;
}


int Http::parse(std::string &packet, const std::string &data) {
    int ret;
    char response[1024]{0};


    std::string method, path, version;

    std::vector<std::string> list = split(data, "\r\n");

    std::istringstream iss(list.front());
    iss >> method >> path >> version;

    list.erase(list.begin());

    std::map<std::string, std::string> obj = getObj(list, ":");
    if (method != "GET") {
        fprintf(stderr, "method = %s\n", method.c_str());
        std::string str = getBody("405 Method Not Allowed");
        sprintf(response,
                "HTTP/1.1 405 Method Not Allowed\r\n"
                "Content-Type: text/html\r\n"
                "Content-Length: %zu\r\n"
                "\r\n"
                "%s",
                str.size(),
                str.c_str()
        );
        ret = TcpSocket::sendData(clientSocket, reinterpret_cast<uint8_t *>(response),
                                  static_cast<int>(strlen(response)));
        if (ret < 0) {
            fprintf(stderr, "发送数据失败\n");
            return ret;
        }
        return -1;
    }
    std::ifstream fs("." + path, std::ios::binary | std::ios::ate);
    if (!fs.good()) {
        fprintf(stderr, "当前文件不可读 %s\n", ("." + path).c_str());
        std::string str = getBody("404 Not Found");
        sprintf(response,
                "HTTP/1.1 404 Not Found\r\n"
                "Content-Type: text/html\r\n"
                "Content-Length: %zu\r\n"
                "\r\n"
                "%s",
                str.size(),
                str.c_str()
        );
        ret = TcpSocket::sendData(clientSocket, reinterpret_cast<uint8_t *>(response),
                                  static_cast<int>(strlen(response)));
        if (ret < 0) {
            fprintf(stderr, "发送数据失败\n");
            return ret;
        }
        return -1;
    }
    std::filesystem::path source("." + path);
    // std::filesystem::path::extension(path);
    std::string mimeType;
    if (source.extension() == ".m3u8") {
        mimeType = "application/x-mpegurl";
    } else if (source.extension() == ".ts") {
        mimeType = "video/mp2t";
    } else {
        fprintf(stderr, "不支持这种类型\n");
        std::string str = getBody("415 Unsupported Media Type");
        sprintf(response,
                "HTTP/1.1 415 Unsupported Media Type\r\n"
                "Content-Type: text/html\r\n"
                "Content-Length: %zu\r\n"
                "\r\n"
                "%s",
                str.size(),
                str.c_str()
        );
        ret = TcpSocket::sendData(clientSocket, reinterpret_cast<uint8_t *>(response),
                                  static_cast<int>(strlen(response)));
        if (ret < 0) {
            fprintf(stderr, "发送数据失败\n");
            return ret;
        }
        return -1;
    }
    uint32_t size = fs.tellg();
    fs.seekg(0, std::ios::beg);
    //   bool flag = fs.eof();
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
    //TcpSocket::closeSocket(clientSocket);
    return 0;
}

Http::~Http() = default;

std::string Http::getBody(const std::string &str) {
    return "<html><body><h1>" + str + "</h1></body></html>";
}
