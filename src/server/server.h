
#ifndef RTSP_SERVER_H
#define RTSP_SERVER_H

#include <cstdint>
#include "TcpSocket.h"
#include "threadPool.h"


class Server {
private:
    bool stopFlag{true};
    std::thread *rtspThread;
    std::thread *httpThread;
    TcpSocket rtsp;
    TcpSocket http;
    /*AnalysisData ad;*/

//    const char *videoFilename{""};
//    const char *audioFilename{""};

    //    NALReader videoReader;
    //AdtsReader audioReader;


//    RtpPacket videoPacket;
//    RtpPacket audioPacket;

    ThreadPool pool;
public:
    int init();

//    int initVideo(const char *filename);
//
//    int initAudio(const char *filename);


    int start();

    ~Server();

private:

    int startRtspServer();

    int startHttpServer();


};


#endif //RTSP_SERVER_H
