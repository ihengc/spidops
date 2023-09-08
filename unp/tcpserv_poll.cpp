#include <sys/poll>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#define OPEN_MAX 1024
#define SERV_PORT 10020
#define MAX_LINE 1024
#define LISTENQ 256
#define INFTIM -1
typedef struct sockaddr SA;

int main(int argc, char *argv[]) {
    int i, maxi, listenfd, connfd, sockfd;
    int nready; // poll()
    ssize_t n; // read(), write()
    struct pollfd client[OPEN_MAX];
    char buf[MAX_LINE];
    struct sockaddr servaddr, cliaddr;
    socklent_t clilen;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERV_PORT);
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    bind(listenfd, (SA *)&servaddr, sizeof(servaddr));

    listen(listenfd, LISTENQ);

    client[0].fd = listenfd;
    client[0].events = POLLRDNORM;

    for(i = 1; i < OPEN_MAX; i++) {
        client[i].fd = -1;
    }
    maxi = 0;
    for( ; ; ) {
        nready = poll(client, OPEN_MAX, INFTIM);
        if(client[0].revents & POLLRDNORM) {
            clilen = sizeof(cliaddr);
            connfd = accept(listenfd, (SA *)&cliaddr, *clilen);
            for(i = 1; i < OPEN_MAX; i++) {
                if(client[i].fd < 0)
                    client[i].fd = connfd;
                    break;
            }
            if(i == OPEN_MAX) {
                printf("too many clients\n");
                exit(-1);
            }
            if(i > maxi)
                maxi = i;
            if(--nready <= 0)
               continue;
        }
        for(i = 1; i <= maxi; i++) {
            if((sockfd = client[i].fd) < 0)
                continue;
            if(client[i].revents & (POLLRDNORM | POLLERR)) {
                if((n = read(sockfd, buf, MAX_LINE)) < 0) {
                    if(errno == ECONNRESET) {
                        close(sockfd);
                        client[i].fd = -1;
                    } else {
                        printf("read error\n");
                        exit(-1);
                    }
                } else if (n == 0) {
                    close(sockfd);
                    client[i].fd = -1;
                } else {
                    write(connfd, buf, n); // echo input message;
                }
                if(--nready <= 0)
                    break;
            }
        }
    }
}
