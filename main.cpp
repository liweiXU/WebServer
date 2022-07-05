#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include "locker.h"
#include "threadpool.h"
#include <signal.h>
#include "http_conn.h"

#define MAX_FD 65536
#define MAX_EVENT_NUMBER 10000

//外部函数
extern int addfd( int epollfd, int fd, bool one_shot);
extern int removefd( int epollfd, int fd );

//argc 终端输入的参数个数，argv 参数的字符串数组的指针
int main (int argc, char* argv[])
{
    if (argc <= 2) {
        //表示缺少参数
        printf("usage: %s ip_address port_number\n", basename(argv[0]));
        return 1;
    }
    //如果输入完整
    const char* ip = argv[1];
    int port = atoi( argv[2] );

    //默认情况下，向一个读端关闭的管道或socket连接中写数据将引发SIGPIPE信号。
    //因为程序接收到SIGPIPE信号的默认行为是结束进程，而我们需要避免因为错误的写程序导致程序退出。
    //因此我们需要捕获处理该信号，或者忽略该信号
    //在这里 我们简单直接使用忽略该信号的方法
    addsig( SIGPIPE, SIG_IGN );

    //创建线程池
    threadpool< http_conn >* pool = NULL;
    try {
        pool = new threadpool< http_conn >;
    }
    catch ( ... )
    {
        return 1;
    }

    //预先为每个可能的客户连接分配一个http_conn对象
    http_conn* users = new http_conn [ MAX_FD ];
    assert( users );
    int user_count = 0;

    //底层协议族 IPv4, 服务类型为流服务，使用TCP协议
    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert( listenfd >= 0 );
    //SO_LINGER选项用于控制close系统调用在关闭TCP连接时的行为
    struct linger tmp = { 1, 0 };
    setsockopt (listenfd, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));

    int ret = 0;
    struct sockaddr_in address;
    bzero( &address, sizeof(address));
    address.sin_family = AF_INET;
    //IP地址转换函数
    inet_pton( AF_INET, ip, &address.sin_addr );
    address.sin_port = htons( port );

    ret = bind (listenfd, ( struct sockaddr* )&address, sizeof(address));
    assert( ret >= 0);

    ret = listen (listenfd, 5);
    assert( ret >= 0);

    epoll_event events[ MAX_EVENT_NUMBER ];
    int epollfd = epoll_create( 5 );
    assert( epollfd != -1 );
    addfd( epollfd, listenfd, false );
    http_conn::m_epollfd = epollfd;

    while (true) {
        int number = epoll_wait( epollfd, events, MAX_EVENT_NUMBER, -1);
        if ((number < 0) && (errno != EINTR))
        {
            printf("epoll failure\n");
            break;
        }

        for (int i = 0; i < number; i++)
        {
            int sockfd = events[i].data.fd;
            if (sockfd == listenfd) {
                struct sockaddr_in client_address;
                socklen_t client_addrlength = sizeof(client_address);
                int connfd = accept (listenfd, (struct sockaddr*)&client_address, &client_addrlength );
                if (connfd < 0)
                {
                    printf("errno is: %d\n", errno);
                    continue;
                }
                if (http_conn::m_user_count >= MAX_FD)
                {
                    show_error(connfd, "Internal server busy");
                    continue;
                }
                //初始化客户连接
                users[connfd].init( connfd, client_address );
            } else if (events[i].events &(EPOLLRDHUP | EPOLLHUP | EPOLLERR))
            {
                //如果有异常，直接关闭客户端连接
                users[sockfd].close_conn();
            } else if (events[i].events & EPOLLIN)
            {
                //根据读到的结果，决定是将任务天假到线程池还是关闭连接
                if (users[sockfd].read()) {
                    pool->append(users + sockfd);
                } else {
                    users[sockfd].close_conn();
                }
            } else if (events[i].events & EPOLLOUT)
            {
                //根据写的结果决定是否关闭连接
                if (!users[sockfd].write())
                {
                    users[sockfd].close_conn();
                }
            }
            else {}
        }
    }
    close( epollfd );
    close( listenfd );
    delete [] users;
    delete pool;
    return 0;
}