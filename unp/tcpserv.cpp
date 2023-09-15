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
#define LISTENQ 256
#define HEADR_LEN 6

int tcp_listen(const char *, uint16_t port);
ssize_t readn(int, char *, size_t);

int main(int argc, char *argv[]) {
    int i, n, sockfd, port, connfd, fds[FD_SETSIZE], maxfdp, connfdcnt;
    fd_set readset;
    struct sockaddr_in cliaddr;
    socklent_ addrlen;
    char addrstr[INET_ADDRSTRLEN], headerbuf[HEADR_LEN];

    if(argc != 3) {
       printf("%s\n", "usage tcpcli <ip> <port>");
       exit(-1);
    }

    port = atoi(argv[2]);
    if(port < 0 || port > (1<<16)-1) {
        printf("port should in [0, 65535], but the value is %d\n", port);
        exit(-1);
    }

    if((sockfd = tcp_listen(argv[1], (uint16_t)port)) < 0) {
        printf("tcp listen %s:%d error\n", argv[1], port);
        exit(-1);
    }

    FD_ZERO(&readset);
    maxfdp = sockfd + 1;
    connfdcnt = 0;
    fds[0] = sockfd;

    for(i = 1; i < FD_SETSIZE; i++) {
        fds[i] = -1;
    }

    for( ; ; ) {
        FD_SET(sockfd, &readset);

        for(i = 1; i < FD_SETSIZE; i++) {
            if(fds[i] == -1)
                continue;
            FD_SET(fds[i], &readset);
        }

        if(select(maxfdp, &readset, NULL, NULL, NULL) < 0) {
            if(errno == EINTR)
                continue;
            else{
                printf("select error: %s\n", strerror(errno));
                break;
            }
        }

        if(FD_ISSET(sockfd, &readset)) {
            addrlen = sizeof(cliaddr);
            if((connfd = accept(sockfd, (struct sockaddr*)&cliaddr, &addrlen)) < 0) {

            } else {
                if(inet_ntop(AF_INET, &cliaddr.sin_addr, addrstr, INET_ADDRSTRLEN) != 1) {
                    printf("%s\n", "inet_ntop parse cli addr error");
                } else {
                    printf("connect by %s:%d\n", addrstr, ntohs(cliaddr.sin_port));
                }

                for(i = 1; i < FD_SETSIZE; i++) {
                    if(fds[i] != -1) {
                        fds[i] = connfd;
                        connfdcnt++;
                        break;
                    }
                }

                if(connfdcnt == FD_SETSIZE - 1) {
                    printf("client too much reach %d\n", FD_SETSIZE - 1);
                    close(connfd);
                }
            }
        }

        for(i = 1; i < FD_SETSIZE; i++) {
            if(FD_ISSET(fds[i], &readset)) { /* readable for connect socket descriptor */
                if((n = readn(fds[i], headerbuf, HEADR_LEN)) <= 0) {
                    close(fds[i]);
                } else {
                    if(n != HEADR_LEN) {
                        continue;
                    }
                    /* unpack header */
                }
            }
        }
    }

    return 0;
}

int tcp_listen(const char *host, uint16_t port) {
    struct sockaddr_in servaddr;
    int sockfd;

    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("%s\n", "socket create socket descriptor error");
        return -1;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    if(inet_pton(AF_INET, host, &servaddr.sin_addr) != 1) {
        printf("inet_pton convert host %s error\n", host);
        return -1;
    }
    servaddr.sin_port = htons(port);

    if(bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        printf("bind %s:%d error\n", host, port);
        return -1;
    }

    if(listen(sockfd, LISTENQ) < 0) {
        printf("listen %s:%d error\n", host, port);
        return -1;
    }
    return sockfd;
}

ssize_t readn(int sockfd, char *buf, size_t n) {
    size_t nleft;
    ssize_t nread;
    char *bp;

    bp = buf;
    nleft = n;
    while(nleft > 0) {
        if((nread = read(sockfd, bp, nleft)) < 0) {
            if(errno == EINTR)
                nread = 0;
            else
                return -1;
        } else if(nread == 0) {
            break;
        } else
            bp += nread;
            nleft -= nread;
    }
    return (n - nleft);
}