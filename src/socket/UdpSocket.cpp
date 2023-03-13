
#include "UdpSocket.h"
#include <cstdio>

int UdpSocket::createSocket() {
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
    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0)
        return -1;
    /*
     * SO_REUSEADDR，打开或关闭地址复用功能
     * 某一端主动关闭会进入一个TIME_WAIT状态，这个时候不能复用这个端口
     * */
    bool on = true;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char *) &on, sizeof(on));

    return 0;
}

int UdpSocket::bindSocket(const char *ip, int port) const {
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
    int ret = bind(sock, (sockaddr *) &addr, sizeof(sockaddr));
    if (ret < 0) {
        fprintf(stderr, "绑定失败");
        return -1;
    }
    return 0;
}
