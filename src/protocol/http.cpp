
#include "http.h"

#include <sstream>

#include "util.h"

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

    std::map<std::string, std::string> obj = getObj(list, ":");
    return 0;
}
