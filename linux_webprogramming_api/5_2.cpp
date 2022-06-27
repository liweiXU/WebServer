#include <bits/socket.h>

struct sockaddr
{
    sa_family_t sa_family;
    unsigned long in _ss_align;
    char _ss_padding[128-sizeof(_ss_align)];
}

