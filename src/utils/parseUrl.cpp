
#include "parseUrl.h"


ParseUrl::ParseUrl(std::string url, int defaultPort) : url_(std::move(url)), port(defaultPort) {

}

int ParseUrl::parse() {
    std::string url = url_;
    size_t pos = url.find("://");
    if (pos != std::string::npos) {
        protocol = url.substr(0, pos);
        url = url.substr(pos + 3);
    } else {
        fprintf(stderr, "解析错误，找不到://");
        return -1;
    }
    pos = url.find(':');
    if (pos != std::string::npos) {
        domain = url.substr(0, pos);
        url = url.substr(pos + 1);
        pos = url.find('/');
        if (pos != std::string::npos) {
            port = std::stoi(url.substr(0, pos));
            url = url.substr(pos);
        } else {
            port = std::stoi(url);
            url = "";
        }
    } else {
        pos = url.find('/');
        if (pos != std::string::npos) {
            domain = url.substr(0, pos);
            url = url.substr(pos);
        } else {
            domain = url;
            url = "";
        }
    }
    path = url;
    return 0;
}

std::string ParseUrl::getProtocol() {
    return protocol;
}

std::string ParseUrl::getDomain() {
    return domain;
}

int ParseUrl::getPort() {
    return port;
}

std::string ParseUrl::getPath() {
    return path;
}
