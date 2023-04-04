#include "server.h"

#include <cstdio>
#include "rtspTask.h"
#include "httpTask.h"


constexpr unsigned short SERVER_RTSP_PORT = 8554;
constexpr unsigned short SERVER_HTTP_PORT = 8080;


int Server::init() {

#if _WIN32
    /*windows 需要初始化socket库*/
    static WSADATA wsadata;
    WSAStartup(MAKEWORD(2, 2), &wsadata);
#endif


    pool.init(20);
    pool.start();

    rtsp.createSocket();
    rtsp.bindSocket("0.0.0.0", SERVER_RTSP_PORT);
    rtsp.listenPort();


    http.createSocket();
    http.bindSocket("0.0.0.0", SERVER_HTTP_PORT);
    http.listenPort();
    return 0;
}

int Server::start() {
    int ret;


    /*std::thread rtspThread(&Server::startRtspServer, this);
    rtspThread.detach();
    std::thread httpThread(&Server::startHttpServer, this);
    httpThread.detach();*/

    rtspThread = new std::thread(&Server::startRtspServer, this);
    httpThread = new std::thread(&Server::startHttpServer, this);


    return 0;
}

int Server::startRtspServer() {
    while (true) {
        SOCKET clientSocket = rtsp.acceptClient();
        if (clientSocket < 0) {
            fprintf(stderr, "accept 失败\n");
            return -1;
        }
        printf("rtsp client ip:%s,client port:%d\n", rtsp.clientIp, rtsp.clientPort);


        RtspTask *task = new RtspTask(clientSocket); // NOLINT(modernize-use-auto)
        pool.addTask(task);

    }
    printf("startRtspServer finish\n");

    return 0;
}

int Server::startHttpServer() {
    while (true) {
        SOCKET clientSocket = http.acceptClient();
        if (clientSocket < 0) {
            fprintf(stderr, "accept 失败\n");
            return -1;
        }
        printf("http client ip:%s,client port:%d\n", rtsp.clientIp, rtsp.clientPort);


        HttpTask *task = new HttpTask(clientSocket); // NOLINT(modernize-use-auto)
        pool.addTask(task);

    }
    printf("startHttpServer finish\n");

    return 0;
}


Server::~Server() {
    printf("server 析构\n");
    stopFlag = false;


    if (rtspThread->joinable()) {
        rtspThread->join();
        delete rtspThread;
    }
    if (httpThread->joinable()) {
        httpThread->join();
        delete httpThread;
    }
    pool.stop();

}








