
#include "http.h"
#include "utils/util.h"
#include "log/logger.h"

#define TAG_SIZE 4
#define TAG_HEADER_SIZE 11


uint8_t Http::response[1024]{0};

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

    if (source.extension() == ".m3u8" || source.extension() == ".ts") {
        ret = hls.init(source, clientSocket);
        if (ret < 0) {
            responseData(500, "错误");
            return -1;
        }
        ret = hls.disposeHls();
        if (ret < 0) {
            responseData(500, "错误");
            return -1;
        }
    } else if (source.extension() == ".flv") {
        /*这里应该单独开个线程去发送数据*/
        sendFlVThread = new std::thread(&Http::sendFLV, this, std::ref(source));

    } else {
        fprintf(stderr, "不支持这种类型\n");
        ret = responseData(415, "Unsupported Media Type");
        return -1;
    }

    return 0;
}

int Http::sendFLV(std::filesystem::path &path) {
    int ret;
    ret = flv.init(path, clientSocket);
    if (ret < 0) {
        responseData(500, "错误");
        return -1;
    }
    ret = flv.disposeFlv();
    if (ret < 0) {
        responseData(500, "错误");
        return -1;
    }
    return 0;
}

int Http::responseData(int status, const std::string &msg) const {
    memset(response, 0, 1024);
    std::string body = "<html><body><h1>" + msg + "</h1></body></html>";
    sprintf(reinterpret_cast<char *>(response),
            "HTTP/1.1 %d %s\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: %zu\r\n"
            "\r\n"
            "%s",
            status, msg.c_str(),
            body.size(),
            body.c_str()
    );

    int ret = TcpSocket::sendData(clientSocket, response, (int) strlen(reinterpret_cast<char *>(response)));
    if (ret < 0) {
        fprintf(stderr, "发送数据失败\n");
        return ret;
    }
    return 0;
}

Http::~Http() {
    log_info("http 析构");

    if (sendFlVThread && sendFlVThread->joinable()) {
        sendFlVThread->join();
    }
}









