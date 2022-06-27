//判断机器字节序

#include <stdio.h>

void byteorder()
{
    union
    {
        short value;
        char union_bytes[ sizeof( short) ];
    } test;

    test.value = 0x0102;
    if ( (test.union_bytes[0] == 1) && (test.union_bytes[1] == 2))
    {
        printf("big edian\n");
    }

    else if ( (test.union_bytes[0] == 2) && (test.union_bytes[1] == 1))
    {
        printf("little endian\n");
    }
    else {
        printf("unknown..\n");
    }
}

int main() {
    byteorder();
    return 0;
}

//Linux提供如下的4个函数来完成主机字节序和网络字节序之间的转换
#include <netinet/in.h>
unsigned long int htonl( unsigned long int hostlong ); //host to network long
unsigned short int htons( unsigned short int hostshort ); 
unsigned long int ntohl( unsigned long int netlong );
unsigned short int ntohs( unsigned short int netshort );
