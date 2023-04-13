
#include "httpHls.h"
#include <fstream>
#include <sstream>


int HttpHls::init(std::filesystem::path &_path, SOCKET socket) {

    if (_path.extension() == ".m3u8") {
        mimeType = "application/x-mpegurl";
    } else if (_path.extension() == ".ts") {
        mimeType = "video/mp2t";
    }
    if (!std::filesystem::exists(_path)) {
        fprintf(stderr, "找不到对应的文件 %ls\n", _path.c_str());
        return -1;
    }
    path = _path.string();
    clientSocket = socket;
    return 0;
}

int HttpHls::disposeHls() {

    int ret;
    std::ifstream fs(path, std::ios::binary | std::ios::ate);
    if (!fs.good()) {
        fprintf(stderr, "当前文件不可读 %s\n", ("." + path).c_str());
        //responseData(500, "当前文件不可读");
        return -1;
    }

    uint32_t size = fs.tellg();
    fs.seekg(0, std::ios::beg);


    std::ostringstream oss;
    oss << "HTTP/1.1 200 OK\r\n";
    oss << "Content-Length: " << size << "\r\n";
    oss << "Content-Type: " << mimeType << "\r\n";
    oss << "Connection: keep-alive\r\n";
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


