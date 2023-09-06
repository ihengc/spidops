#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h> // htons()
#include <string.h> /* memset(), strerror()*/
#include <stdlib.h> // exit()
#include <unistd.h> /* read()*/
#include <stdio.h> /* printf() */
#define SERV_PORT 10020
#define BACKLOG 256
#define MAXLINE 1024
typedef struct sockaddr SA;

void err_quit(const char *msg) {
    printf("%s\n", msg);
    exit(-1);
}

void err_sys(const char *msg) {
    printf("%s\n");
}

ssize_t writen(int sockfd, const void *buf, size_t n) {
    size_t nleft = n;
    ssize_t nwritten;
    const char *p = (char *)buf;
    while(nleft > 0) {
        if((nwritten = write(sockfd, p, nleft)) <= 0) {
            if(nwritten < 0 && errno == EINTR) {
                nwritten = 0;
            } else {
                return -1;
            }
        }
        nleft -= nwritten;
        p += nwritten;
    }
    return (n);
}

void str_echo(int sockfd) {
    char buf[MAXLINE];
    size_t n;
again:
    while((n=read(sockfd, buf, MAXLINE)) > 0) {
        writen(sockfd, buf, n);
    }
    if(n < 0 && errno == EINTR) {
        goto again;
    } else if (n < 0) {
        err_sys("str_echo: read error");
    }
}

int main(int argc, char *argv[]) {
    int listenfd, connfd, n;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t clilen;
    char clihost[INET_ADDRSTRLEN+1], msg[50];
    pid_t childpid;

    if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        err_quit("socket error.");
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERV_PORT);
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if((n = bind(listenfd, (SA *)&servaddr, sizeof(servaddr))) == -1) {
        /* addr reused */
        err_quit("bind error");
    }
    if((n = listen(listenfd, BACKLOG)) == -1) {
        err_quit("listen error");
    }
    for( ; ; ) {
        clilen = sizeof(cliaddr);
        if((connfd = accept(listenfd, (SA *)&cliaddr, &clilen)) == -1) {
            if(errno == EINTR || errno == ECONNABORTED) {
                continue;
            } else {
                err_quit("accept error.");
                break;
            }
            if(inet_ntop(AF_INET, &cliaddr.sin_addr, clihost, INET_ADDRSTRLEN+1) == NULL) {
                err_sys("inet_ntop error");
            } else {
                sprintf(msg, "connect by %s:%d", clihost, ntohs(cliaddr.sin_port));
                err_sys(msg);
            }
            if((childpid = fork()) == 0) { /* child process */
                close(listenfd);
                str_echo(connfd);
                exit(0);
            }
            close(connfd);
        }
    }
    return 0;
}