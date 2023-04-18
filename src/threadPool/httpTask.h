
#ifndef RTSP_HTTPTASK_H
#define RTSP_HTTPTASK_H

#include "task.h"
#include "socket/TcpSocket.h"

class HttpTask : public Task {
private:
    SOCKET clientSocket;
public:
    explicit HttpTask(SOCKET socket);

    int run() override;

    ~HttpTask();
};


#endif //RTSP_HTTPTASK_H
