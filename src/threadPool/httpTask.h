
#ifndef RTSP_HTTPTASK_H
#define RTSP_HTTPTASK_H

#include "task.h"
#include "TcpSocket.h"

class HttpTask : public Task {
private:
    SOCKET clientSocket;
public:
    explicit HttpTask(SOCKET socket);
    int run() override;
};


#endif //RTSP_HTTPTASK_H
