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
2) readfds, writefds和exceptfds参数分别指向可读，可写和异常等事件对应的文件描述符集合。应用程序调用select函数时，通过这三个参数传入自己感兴趣的文件描述符select调用返回时，内核将修改它们来通知应用程序哪些文件描述符已经就绪。
3) timeout参数用来设置select函数的超时时间，它是一个timeval结构类型的指针，采用指针参数是因为内核将修改它以告诉应用程序select等待了多久。
select成功时返回就绪(可读，可写和异常)文件描述符的总数，如果在超时时间内没有任何文件描述符就绪，select将返回0。select失败时返回-1并设置errno。如果在select等待期间，程序接收到信号，则select立刻返回-1，并设置errno为EINTR。

 ```cpp
 #include <poll.h>
 int poll(struct pollfd* fds, nfds_t nfds, int timeout);
 ```
 1) fds参数是一个pollfd结构类型的数组，它指定所有我们感兴趣的文件描述符上发生的可读，可写和异常等事件。
 ```cpp
 sruct pollfd
 {
    int fd; //文件描述符
    short events; //注册的事件
    short revents; //实际发生的事件，由内核填充
 }
 ```
 其中，fd成员指定文件描述符；events成员告诉poll监听fd上的哪些事件，它是一系列事件的按位或；revent成员则由内核修改，以通知应用程序fd上实际发生了哪些事情。 
 nfds与timeout的定义同上。

 epoll是Linux特有的I/O复用函数。它在实现上与select，poll有很大差异。首先，epoll使用一组函数来完成任务，而不是单个函数。其次，epoll把用户关心的文件描述符上的事件放在内核里的一个事件表里，从而无须像select和poll那样每次调用都要重复传入文件描述符集和事件集。但是epoll需要一个额外的文件描述符，来唯一标识内核中的这个事件表。这个文件描述符使用epoll_create函数来创建:
 ```cpp
 #include <sys/epoll.h>
 int epoll_create( int size )
 ```
 该函数返回的文件描述符将用作其他所有epoll系统调用的第一个参数，以指定要访问的内核事件表。
 ```cpp
 #include <sys/epoll.h>
 int epoll_ctl( int epfd, int op, int fd, struct epoll_event *event )
 ```
 fd参数是要操作的文件描述符，op参数则指定操作类型。操作类型有:
 EPOLL_CTL_ADD 往事件表中注册fd上的事件
 EPOLL_CTL_MOD 修改fd上的注册事件
 EPOLL_CTL_DEl 删除fd上的注册事件
 epoll系列系统调用的主要接口是epoll_wait函数，它在一段超时事件内等待一组文件描述符上的事件。
 ```cpp
 #incldue <sys/epoll.h>
 int epoll_wait( int epfd, struct epoll_event* events, int maxevents, int timeout );
 ```
 epoll_wait函数如果检测到事件，就将所有就绪的事件从内核事件表中复制到它的第二个参数events指向的数组中。这个数组只用于输出epoll_wait检测到的就绪事件，而不像select和poll的数组参数那样既用于传入用户注册的事件，又用于输出内核检测到的就绪事件，这就极高地提高了应用程序索引就绪文件描述符的效率。
 epoll对文件描述符的操作有两种模式: LT(Level Trigger, 电平触发)模式和ET(Edge Trigger，边沿触发)模式。LT模式是默认的工作模式，这种情况下epoll相当于一个效率较高的poll。当往epoll内核事件表中注册一个文件描述符上的EPOLLET事件时，epoll将以ET模式来操作该文件描述符。ET模式是epoll的高效工作模式。
 对于采用LT工作模式的文件描述符，当epoll_wait检测到其上有事件发生并将此事件通知应用程序后，应用程序可以不立即处理该事件。这样，当应用程序下一次调用epoll_wait时，epoll_wait还会再次向应用程序通报次事件，直到该事件被处理。而对于采用ET工作模式的文件描述符，当epoll_wait检测到其上有事件发生并将此一件通知应用程序后，应用程序必须立即处理该事件，因为后续的epoll_wait不再向应用程序通知这一事件。ET模式在很大程度上见底了同一个epoll事件被重复触发的次数，因此效率比LT模式高。
 select, poll和epoll都通过某种结构体变量来告诉内核监听哪些文件描述符上的哪些事件，并使用该结构体类型的参数来获取内核处理的结果。
 select的参数类型fd_set没有将文件描述符和事件绑定，它仅仅是一个文件描述符的集合，因此select需要提供3个这种类型的参数来分别传入和输出可读，可写及异常等事件。这一方面使得select不能处理更多类型的事件，另一方面，由于内核对fd_set集合的在线修改，应用程序瑕疵调用select前不得不重置这3个fd_set集合。