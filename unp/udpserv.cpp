#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#define MAXLINE 1024
#define LISTENQ 256
#define SERV_PORT 10021

typedef struct sockaddr SA;
int main(int argc, char *argv[]) {
    int sockfd, connfd;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t clilen;
    char buf[MAXLINE];
    ssize_t nread;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERV_PORT);
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    bind(sockfd, (SA *)&servaddr, sizeof(servaddr));
    listen(sockfd, LISTENQ);

    for( ; ; ) {
        clilen = sizeof(cliaddr);
        nread = recvfrom(sockfd, buf, MAXLINE, 0, (SA *)&cliaddr, &clilen);
        if(nread >= 0) {
            sendto(sockfd, buf, (size_t)nread, 0, (SA *)&cliaddr, clilen);
        } else {
            printf("recvfrom error: %s\n", strerror(errno));
            continue;
        }
    }
    return 0;
}

