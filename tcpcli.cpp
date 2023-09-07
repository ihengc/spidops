#include <netinet/in.h>
#include <sys/socket.h>
#include <string.h>
#include <stdio.h>
#include <sys/select.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#define MAX_LINE 1024
#define MSG_LEN 60
typedef struct sockaddr SA;

void err_quit(const char *msg) {
    printf("%s\n", msg);
    exit(-1);
}

ssize_t writen(int fd, void *buf, size_t maxlen) {
    ssize_t nwritten;
    size_t nleft = maxlen;
    const char *p = (char *)buf;

    while(nleft > 0) {
        if((nwritten = write(fd, p, nleft)) <= 0) {
            if(nwritten < 0 && errno == EINTR)
                nwritten = 0
            else
                return -1;
        }
        nleft -= nwritten;
        p += nwritten;
    }
    return maxlen;
}

void str_cli(FILE *fp, int sockfd) {
    fd_set rset;
    char buf[MAX_LINE];
    int maxfdp1, n, stdineof;

    stdineof = 0;
    FD_ZERO(&rset);
    maxfdp1 = (fileno(fp) >= sockfd ? fileno(fp) : sockfd) + 1;
    for ( ; ; ) {
        FD_SET(fileno(fp), &rset);
        FD_SET(sockfd, &rset);
        if((n=select(&rset, NULL, NULL, NULL)) <= 0) {
            if(errno == EINTR) {
                continue;
            } else {
                err_quit("str_cli: select error");
            }
        } else {
            if(FD_ISSET(sockfd, &rset)) {
                if((n = read(sockfd, buf, MAX_LINE)) == 0)  {
                    if(stdineof == 1)
                        return ;
                    else
                        err_quit("str_cli: server terminated prematurely");
                }
                write(fileno(stdout), buf, n);
            }
            if(FD_ISSET(fileno(fp), &rset)) {
                if((n = read(fileno(fp), buf, MAX_LINE)) == 0) {
                    stdineof = 1;
                    shutdown(sockfd, SHUT_WR);
                    FD_CLR(fileno(fp), &rset);
                    continue;
                }
                writen(sockfd, buf, n);
            }
        }
    }
}

int main(int argc, char *argv[]) {
    int sockfd, n, port;
    struct sockaddr_in servaddr;
    char msg[MSG_LEN];

    if(argc != 3) {
        err_quit("usage tcpcli <IPAddress> <Port>");
    }
    port = atoi(argv[2]);
    if(port < 0 || port > (1<<16)-1) {
        sprintf(msg, "port error:%d", port);
        err_quit(msg);
    }
    if((sockfd=socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        err_quit("socket error");
    }
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons((uint16_t)port);

    if((n = inet_pton(AF_INET, argv[1], &servaddr.sin_addr)) <= 0) {
        sprintf(msg, "host error:%s", argv[1]);
        err_quit(msg);
    }
    if((n = connect(sockfd, (SA *)&servaddr, sizeof(servaddr))) == -1) {
        sprintf(msg, "connect error:", strerror(errno));
        err_quit(msg);
    } else {
        str_cli(stdin, sockfd);
        exit(0);
    }
    return 0;
}