/* udp client */
#include <netinet/in.h>
#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#define MAXLINE 1024
void datagram_cli(FILE *fp, int sockfd, const struct sockaddr *pservaddr, socklen_t servlen);

int main(int argc, char *argv[]) {
    int sockfd, port, n;
    struct sockaddr_in servaddr;

    if(argc != 3) {
        printf("usage udpcli <IPaddress> <Port>\n");
        exit(-1);
    }
    port = atoi(argv[2]);
    if(port < 0 || port > (1<<16)-1) {
        printf("port error: %d\n", port);
        exit(-1);
    }
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons((uint16_t)port);
    if((n=inet_pton(AF_INET, argv[1], &servaddr.sin_addr)) <= 0) {
        printf("inet_pton error: %s\n", argv[1]);
        exit(-1);
    }
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    datagram_cli(stdin, sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
    return 0;
}

void datagram_cli(FILE *fp, int sockfd, const struct sockaddr *pservaddr, socklen_t servlen) {
    int n;
    char sendline[MAXLINE], recvline[MAXLINE];
    /*若当前udp服务器未开启，则会阻塞待fgets上，sendto，recvfrom返回错误*/
    while(fgets(sendline, MAXLINE, fp) != NULL) { /*从标准输入中读取数据*/
        sendto(sockfd, sendline, strlen(sendline), 0, pservaddr, servlen); /*发送数据*/
        /*todo 我们并不知道recvfrom接收的数据是否来自我们发送的服务器，我们需要接收对端的地址数据并进行比较*/
        recvfrom(sockfd, recvline, MAXLINE, 0, NULL, NULL); /*接收服务器的应答，这里服务器地址和长度设置NULL，表示不关心其地址*/
        fputs(recvline, stdout); /*将服务器应答送往标准输出*/
    }
}