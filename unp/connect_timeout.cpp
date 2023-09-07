#include <netinet/in.h>
static void connect_alarm(int);

int connect_timeout(int sockfd, const sockaddr * saptr, socklen_t salen, int nsec) {
    Sigfunc *sigfunc; /* 信号处理函数 */
    int n;

    sigfunc = Signal(SIGALRM, connect_alarm); /* 注册信号处理函数 */
    if(alarm(nsec) != 0) /* 在指定nsec时间后，产生SIGALRM信号 */
        err_msg("connect_timeout: alarm was already set");
    if((n=connect(sockfd, saptr, salen)) < 0) {
        close(sockfd);
        if(errno == EINTR)
            errno = ETIMEDOUT;
    }
    alarm(0); /* 关闭进程的报警时钟 */
    Signal(SIGALRM, sigfunc);
    return (n);
}

static void connect_alarm(int signo) {
    return;
}