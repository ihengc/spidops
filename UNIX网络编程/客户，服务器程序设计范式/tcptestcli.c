/* tcp测试用客户程序 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#define NAXN 16384 /* 最大请求大小（字节） */
#define MAXLINE 1024

int main(int argc, char **argv) {
    int i, j, fd, nchildren, nloops, nbytes;
    pid_t pid;
    ssize_t n;
    char request[MAXLINE], reply[MAXN];

    if(argc != 6) {
        printf("usage: tcptestcli <hostname or IPaddr> <port> <#children> "
               "<#loops/child> <#bytes/request>\n");
        exit(-1);
    }
    nchildren = atoi(argv[3]);
    nloops = atoi(argv[4]);
    nbytes = atoi(argv[5]);

    snprintf(request, sizeof(request), "%d\n", nbytes);

    for(i = 0; i < nchildren; i++) {
        if((pid = fork()) == 0) { /* child */
            for(j = 0; j < nloops; j++) {
                fd = tcp_connect(argv[1], argv[2]);
                write(fd, request, strlen(request));
                if((n = readn(fd, reply, nbytes)) != nbytes) {
                    printf("server returned %d bytes\n", n);
                    exit(-1);
                }
                close(fd);
            }
            printf("child %d done\n", i);
            exit(0);
        }
    }

    while(wait(NULL) > 0)
        ;

    if(errno != ECHILD) {
        printf("wait error\n");
    }

    exit(0);
}