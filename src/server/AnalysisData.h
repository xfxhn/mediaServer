

#ifndef RTSP_ANALYSISDATA_H
#define RTSP_ANALYSISDATA_H

#include <cstdint>
#include <map>
#include <vector>
#include <string>
#include <cstring>
#include "adtsHeader.h"
#include "TcpSocket.h"
#include "task.h"

class AnalysisData {
private:
    struct {
        std::string version;
        std::map<std::string, std::string> origin;
        std::string name;
        std::map<std::string, std::string> timing;
        std::map<std::string, std::string> media[2];
    } sdpInfo;

    std::string packet;
    char buffer[TcpSocket::MAX_BUFFER]{0};
public:
    /*uint16_t SERVER_RTP_PORT{0};
    uint16_t SERVER_RTCP_PORT{0};*/


    /*uint16_t clientRtpPort{0};
    uint16_t clientRtcpPort{0};*/


    uint16_t audioClientRtpPort{0};
    uint16_t audioClientRtcpPort{0};

    uint8_t videoChannel{0};
    uint8_t audioChannel{0};
    uint16_t videoClientRtcpPort{0};
    const char *clientIp{""};

    AdtsHeader header;

    std::string aacConfig;
    std::string sprop_parameter_sets;
    std::string profileLevelId;

public:

    int getInfo(TcpSocket &rtsp, const char *requestBuffer);

    int receiveRtspData(TcpSocket &rtsp);

    int test(TcpSocket &rtsp);

    int test1(TcpSocket &rtsp, Task *task);

private:
    int parseSdp(const std::string &sdp);

    int parseMediaLevel(int i, const std::vector<std::string> &list);

    int receiveResidualData(int size);

    static char *getLine(char *buf, char *line);

    /*static std::list<std::string> split(const std::string &str, const std::string &spacer);*/

    static std::map<std::string, std::string> getRtspObj(std::vector<std::string> &list);
};


#endif //RTSP_ANALYSISDATA_H
