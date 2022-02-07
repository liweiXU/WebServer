#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H

#include <sys/epoll.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include "locker.h"
#include <sys/uio.h>


class http_conn {
public:

    static int m_epollfd; //所有的socket上的事件都被注册到同一个epoll对象中
    static int m_user_count; //统计用户的数量
    static const int READ_BUFFER_SIZE = 2048; //读缓冲区的大小
    static const int WRITE_BUFFER_SIZE = 1024; //写缓冲区的大小
    
    http_conn() {}
    ~http_conn() {}

    void process(); // 处理客户端请求

    void init(int sockfd, const sockaddr_in & addr); //initialize new conn

    void close_conn(); //close cnn

    bool read(); //nonblocking read

    bool write(); //nonblocking write

private:
    int m_sockfd; // 该HTTP连接的socket
    sockaddr_in m_address; //通信的socket地址
    char m_read_buf[READ_BUFFER_SIZE]; //读缓冲区
    int m_read_idx; //标识读缓冲区中以及读入的客户端数据的最后一个字节的下一个位置
};

#endif