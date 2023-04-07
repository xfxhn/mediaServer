
#include "TcpSocket.h"
#include <cstring>
#include <cstdio>


int TcpSocket::createSocket() {
    /*
    参数1：一个地址描述，即地址类型，我们选择 IPv4(AF_INET)
    参数2：指定socket类型，我们选择流式套接字(SOCK_STREAM)
    参数3：指定socket所用的协议，当然是 TCP 协议了(IPPROTO_TCP)
     */
    /*
流格式套接字（Stream Sockets）也叫“面向连接的套接字”，在代码中使用 SOCK_STREAM 表示。
SOCK_STREAM 是一种可靠的、双向的通信数据流，数据可以准确无误地到达另一台计算机，如果损坏或丢失，可以重新发送
     */

    /*获取 socket 描述符*/
    serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket < 0)
        return -1;
    /*
     * SO_REUSEADDR，打开或关闭地址复用功能
     * 某一端主动关闭会进入一个TIME_WAIT状态，这个时候不能复用这个端口
     * */
    bool on = true;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (const char *) &on, sizeof(on));

    return 0;
}


int TcpSocket::bindSocket(const char *ip, int port) const {
    sockaddr_in addr{};
    /*地址描述*/
    addr.sin_family = AF_INET;
    /*端口信息  需要将 “主机字节顺序” 转换到 “网络字节顺序” ，而 htons () 函数就是用来做这种事情的*/
    addr.sin_port = htons(port);
    /*inet_addr () 则是将 “十进制点分ip” 转换为一个符合结构体格式的整数，而其转换后即为 “网络字节顺序”*/
    addr.sin_addr.s_addr = inet_addr(ip);
    /*
    inet_addr() 将 十进制点分ip 转换为 网络字节顺序
    inet_ntoa() 将 网络字节顺序 转换为 十进制点分ip

    htons() 将 short 主机字节顺序 转换为 网络字节顺序
    ntohs() 将 short 网络字节顺序 转换为 主机字节顺序

    ntohl（）将 long 主机字节顺序 转换为 网络字节顺序
    htonl（）将 long 网络字节顺序 转换为 主机字节顺序
     */

    /*
    参数1：socket 描述符
    参数2：一个结构体，包含服务端的 IP 地址及端口信息
    参数3：第二个参数的长度
     */
    int ret = bind(serverSocket, (sockaddr *) &addr, sizeof(sockaddr));
    if (ret < 0) {
        fprintf(stderr, "绑定失败");
        return -1;
    }
    return 0;
}

int TcpSocket::listenPort(int backlog) const {

    /*
这时候我们需要调用 listen ()，对我们刚才对结构体赋值的时候填上去的端口进行监听。
首先说明白，这个函数的意义是开始监听，开始监听后该函数将直接返回，而不是监听直到用户进入*/
/*
参数1：socket 描述符
参数2：进入队列中允许的连接数目
参数2是进入队列即在你调用 accept() 同意客户接入之前的客户等待队列。
同一时间可能有很多客户端请求连接，若超出了服务端的处理速度，进入队列会很长，
那么多于你定义的队列长度的客户就会被 socket底层 直接忽略掉。
	 */
    int ret = listen(serverSocket, backlog);
    if (ret < 0) {
        fprintf(stderr, "监听失败\n");
        return -1;
    }
    return 0;
}


SOCKET TcpSocket::acceptClient() {
    sockaddr_in addr{};

    int len = sizeof(addr);
    /*接受客户的连接请求*/
    SOCKET clientSocket = accept(serverSocket, (sockaddr *) &addr, reinterpret_cast<socklen_t *>(&len));
    if (clientSocket < 0)
        return -1;

    /*inet_ntoa() 将 网络字节顺序 转换为 十进制点分ip*/
    strcpy(clientIp, inet_ntoa(addr.sin_addr));
    /*ntohs() 将 short 网络字节顺序 转换为 主机字节顺序*/
    clientPort = ntohs(addr.sin_port);

    return clientSocket;
}


int TcpSocket::receive(SOCKET clientSocket, char *data, int &dataSize) {


    dataSize = recv(clientSocket, data, MAX_BUFFER, 0);
    /*这里返回0的话表示对端关闭了连接，发送了FIN包*/
    if (dataSize <= 0) {
        fprintf(stderr, "接收数据失败 错误值 = %d\n", dataSize);
        return -1;
    }
    return 0;
}


int TcpSocket::sendData(SOCKET clientSocket, uint8_t *buffer, int length) {

    int ret = send(clientSocket, reinterpret_cast<const char *>(buffer), length, 0);
    if (ret < 0) {
        fprintf(stderr, "发送失败\n");
        return ret;
    }
    return 0;
}


int TcpSocket::closeSocket(SOCKET clientSocket) {
#if _WIN32
    closesocket(clientSocket);
#else
    close(clientSocket);
#endif
    return 0;
}

TcpSocket::~TcpSocket() {
#if _WIN32
    closesocket(serverSocket);
#else
    close(serverSocket);
#endif
}





