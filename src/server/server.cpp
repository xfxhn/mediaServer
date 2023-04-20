#include "server.h"

#include "threadPool/rtspTask.h"
#include "threadPool/httpTask.h"
#include "log/logger.h"

int Server::init(int rtspPort, int httpPort) {
    int ret;
#if _WIN32
    /*windows 需要初始化socket库*/
    static WSADATA wsadata;
    WSAStartup(MAKEWORD(2, 2), &wsadata);
#endif
    Logger::getInstance()->setLevel(LOG_TRACE);

    pool.init(20);
    pool.start();

    rtsp.createSocket();
    ret = rtsp.bindSocket("0.0.0.0", rtspPort);
    if (ret < 0) {
        return -1;
    }

    rtsp.listenPort();
    log_info("启动RTSP，监听%d端口", rtspPort);

    http.createSocket();
    ret = http.bindSocket("0.0.0.0", httpPort);
    if (ret < 0) {
        return -1;
    }
    http.listenPort();
    log_info("启动HTTP，监听%d端口", httpPort);
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
            log_error("rtsp accept 失败");
            return -1;
        }
        log_info("rtsp 连接  ip=%s, port=%d", rtsp.clientIp, rtsp.clientPort);
        RtspTask *task = new RtspTask(clientSocket); // NOLINT(modernize-use-auto)
        pool.addTask(task);

    }
    return 0;
}

int Server::startHttpServer() {
    while (true) {
        SOCKET clientSocket = http.acceptClient();
        if (clientSocket < 0) {
            log_error("http accept 失败");
            return -1;
        }
        log_info("http 连接  ip=%s, port=%d", http.clientIp, http.clientPort);
        HttpTask *task = new HttpTask(clientSocket); // NOLINT(modernize-use-auto)
        pool.addTask(task);

    }

    return 0;
}


Server::~Server() {


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








