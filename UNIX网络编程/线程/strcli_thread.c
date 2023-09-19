#include <unistd.h>
#include <stdlib.h>
#include <pthread>
#include <errno.h>
#define MAXLINE 1024;

void *copyto(void *); /* 线程执行的目标函数 */
static int sockfd; /* 全局静态变量 */
static FILE *fp;

void str_cli(FILE *fp_arg, int sockfd_arg) {
    char recvline[MAXLINE]; /* 接收缓冲区 */
    pthread_t tid; /* 线程id */
    int error; /* 用于保存创建线程时返回的错误码 */

    sockfd = sockfd_arg;
    fp = fp_arg;

    /* 启动线程执行copyto函数，不传递任何参数，使用线程默认属性 */
    if((error = pthread_create(&tid, NULL, copyto, NULL)) != 0) {
        printf("pthread_create function error: %s\n", strerror(error));
        exit(-1);
    }
    while(readline(sockfd, recvline, MAXLINE) > 0) /* 读取从套接字种一行数据，并把数据写到标准输出中 */
        fputs(recvline, stdout);
}

void *copyto(void *arg) {
    char sendline[MAXLINE];

    while(fgets(sendline, MAXLINE, fp) != NULL)
        writen(sockfd, sendline, strlen(sendline)); /* 写 */

    shutdown(sockfd, SHUT_WR);

    return (NULL);
}