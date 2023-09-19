/* 线程版TCP回射服务器程序 */

int main(int argc, char *argv[]) {
    int listenfd, connfd;
    pthread_t tid;
    socklen_t addrlen, len;
    struct sockaddr *cliaddr;

    if(argc == 2)
        listenfd = tcp_listen(NULL, argv[1], &addrlen);
    else if (argc == 3)
        listenfd = tcp_listen(argv[1], argv[2], &addrlen);
    else {
        printf("usage: tcpechoserv_thread [ <host> ] <service or port>\n");
        exit(-1);
    }

    cliaddr = malloc(addrlen);

    for( ; ; ) {
        len = addrlen;
        connfd = accept(listenfd, cliaddr, &len);
        pthread_create(&tid, NULL, &doit, (void *)connfd);
    }
}

static void doit(void *arg) {
    pthread_detach(pthread_self());
    str_echo((int)arg);
    close((int)arg);
    return (NULL);
}