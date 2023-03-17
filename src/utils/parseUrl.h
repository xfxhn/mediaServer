

#ifndef RTSP_PARSEURL_H
#define RTSP_PARSEURL_H

#include <string>

class ParseUrl {
private:

    std::string url_;
    std::string protocol;
    std::string domain;
    std::string path;
    int port;
public:

    explicit ParseUrl(std::string url, int defaultPort);


    int parse();

    std::string getProtocol();

    std::string getDomain();

    std::string getPath();

    int getPort();

};


#endif //RTSP_PARSEURL_H
