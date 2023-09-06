#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <errno.h>
#define MAXLINE 1024
#define SERV_PORT 10020
typedef struct sockaddr SA;

void err_quit(const char *msg) {
    printf("%s\n", msg);
    exit(-1);
}

void err_sys(const char *msg) {
    printf("%s\n");
}

ssize_t writen(int sockfd,void *buff, size_t n) {
    size_t nleft = n;
    ssize_t nwritten;
    const char *p = (char *)buff;

    while((nwritten = write(sockfd, p, nleft)) <= 0) {
        if(nwritten < 0 && errno == EINTR)
            nwritten = 0;
        else
            return -1;
        nleft -= nwritten;
        p += nwritten;
    }
    return n;
}

static int read_cnt;
static char *read_ptr;
static char read_buff[MAXLINE];

static ssize_t my_read(int fd, char *p) {
    if(read_cnt <= 0) {
        again:
            if((read_cnt = read(fd, read_buff, sizeof(read_buff))) < 0) {
                if(errno == EINTR)
                    goto again;
                return -1;
            } else if(read_cnt == 0)
                return 0;
            read_ptr = read_buff;
    }
    read_cnt--;
    *p = *read_ptr++;
    return 1;
}



ssize_t readline(int sockfd, void *buff, size_t maxlen) {
    size_t n, rc;
    char c, *p;

    p = (char *)buff;
    for(n = 1; n < maxlen; n++) {
        if((rc = my_read(sockfd, &c)) == 1) {
            *p++ = c;
            if(c == '\n')
                break;
        } else if(rc == 0) {
            *p = 0;
            return (n - 1);
        } else {
            return -1;
        }
    }
    *p = 0;
    return n;
}

void str_cli(FILE *fdp, int sockfd) {
    char sendline[MAXLINE], recvline[MAXLINE];

    while(fgets(sendline, MAXLINE, fdp) != NULL) {
        writen(sockfd, sendline, strlen(sendline));
        if(readline(sockfd, recvline, MAXLINE) == 0)
            err_quit("str_cli: server terminated permaturely");
        fputs(recvline, stdout);
    }
}

int main(int argc, char *argv[]) {
    int sockfd, n;
    struct sockaddr_in servaddr;
    char msg[60];

    if(argc != 2) {
        err_quit("usage: tcpcli <IPaddress>");
    }
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        err_quit("socket error");
    }
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERV_PORT);
    inet_pton(AF_INET, argv[1], &servaddr.sin_addr);

    if((n = connect(sockfd, (SA *)&servaddr, sizeof(servaddr))) == -1) {
        sprintf(msg, "connect %s:%d error", argv[1], SERV_PORT);
        err_quit(msg);
    }
    str_cli(stdin, sockfd);
    exit(0);
}