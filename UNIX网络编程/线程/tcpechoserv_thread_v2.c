static void doit(void *);

int main(int argc, char *argv[]) {
    int listenfd, *iptr;
    pthread_t tid;
    socklen_t addrlen, len;
    struct sockaddr *cliaddr;

    if(argc == 2)
        listenfd = tcp_listen(NULL, argv[1], &addrlen);
    else if(argc == 3)
        listenfd = tcp_listen(argv[1], argv[2], &addrlen);
    else {
        printf("usage: tcpechoserv_thread_v2 [ <host> ] <service or port>\n");
        exit(-1);
    }
    cliaddr = malloc(addrlen);

    for( ; ; ) {
        len = addrlen;
        iptr = malloc(sizeof(int)); /* 每个线程都会重新分配一块内存，不会存在多个线程访问同一块内存的情况 */
        *iptr = accept(listenfd, cliaddr, &len);
        pthread_create(&tid, NULL, &doit, iptr);
    }
}

static void *doit(void *arg) {
    int connfd;

    connfd = *((int*)argc);
    free(arg);

    pthread_detach(pthread_self());
    str_echo(connfd);
    close(connfd);
    return (NULL);
}