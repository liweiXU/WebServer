# WebServer
This project is mainly based on <<Linux高性能服务器编程>>, combined with some extra improvings.

## 有限状态机
在项目中，使用了两个有限状态机，分别称为主状态机和从状态机。主状态机在内部调用从状态机。
主状态机使用checkstate变量来记录当前状态。如果当前状态是CHECK_STATE_REQUESTLINE, 则表示parse_line函数解析出的行是请求行，于是主状态机调用parse_requestline来分析请求行；如果当前的状态是CHECK_STATE_HEADER，则表示pars_line函数解析出的是头部字段，从而状态机调用parse_headers来分析头部字段。checkstate变量的初始之为CHECK_STATE_REQUESTLINE，parse_requestline函数在成功地分析完请求行之后将其设置为CHECK_STATE_HEADER, 从而实现状态转移。

从状态机，即parse_line函数。它的初始状态是LINE_OK，其原始驱动力来自于buffer中新到达的客户数据。在main函数中，我们循环调用recv函数往buffer中读入客户数据。每次成功读取数据后，我们就调用parse_content函数来分析新读入的数据。parse_content函数首先要做的就是调用parse_line函数来获取一个行。


## I/O复用
```cpp
#include <sys/select.h>
int select(int nfds, fd_set* readfds, fd_set* writefds, 
    fd_set* exceptfds, struct timeval* timeout);
```
1) nfds参数指定被监听的文件描述符的总数。它通常被设置为select监听的所有文件描述符中的最大值加1，因为文件描述符是从0开始计数的。
2）readfds, writefds和exceptfds参数分别指向可读，可写和异常等事件对应的文件描述符集合。应用程序调用select函数时，通过这三个参数传入自己感兴趣的文件描述符select调用返回时，内核将修改它们来通知应用程序哪些文件描述符已经就绪。
3) timeout参数用来设置select函数的超时时间，它是一个timeval结构类型的指针，采用指针参数是因为内核将修改它以告诉应用程序select等待了多久。
select成功时返回就绪(可读，可写和异常)文件描述符的总数，如果在超时时间内没有任何文件描述符就绪，select将返回0。select失败时返回-1并设置errno。如果在select等待期间，程序接收到信号，则select立刻返回-1，并设置errno为EINTR。