
#ifndef RTSP_SERVER_H
#define RTSP_SERVER_H

#include <cstdint>
#include "socket/TcpSocket.h"
#include "threadPool/threadPool.h"


class Server {
private:
    std::thread *rtspThread;
    std::thread *httpThread;
    TcpSocket rtsp;
    TcpSocket http;


    ThreadPool pool;
public:
    int init(int rtspPort, int httpPort);

    int start();

    ~Server();

private:

    int startRtspServer();

    int startHttpServer();


};


#endif //RTSP_SERVER_H
