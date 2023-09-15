#include <netinet/in.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#define DEFAULT_CONN_TIMEOUT 5

int tcp_connect(const char *, uint16_t, int);

int main(int argc, char *argv[]) {
    int sockfd, port;

    if(argc != 3) {
       printf("%s\n", "usage tcpcli <ip> <port>");
       exit(-1);
    }

    port = atoi(argv[2]);
    if(port < 0 || port > (1<<16)-1) {
        printf("port should in [0, 65535], but the value is %d\n", port);
        exit(-1);
    }

    if((sockfd = connect(argv[1], (uint16_t)port, DEFAULT_CONN_TIMEOUT)) < 0) {
        if(errno == ETIMEDOUT)
            printf("connect %s:%d timeout", argv[1], port);
        else
            printf("connect %s:%d fail", argv[1], port);
        exit(-1);
    }
    return 0;
}