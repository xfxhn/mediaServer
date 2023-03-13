
#ifndef RTSP_SERVER_H
#define RTSP_SERVER_H

#include <cstdint>
#include "TcpSocket.h"
#include "threadPool.h"


class Server {
private:
    TcpSocket rtsp;
    /*AnalysisData ad;*/

    const char *videoFilename{""};
    const char *audioFilename{""};

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


};


#endif //RTSP_SERVER_H
