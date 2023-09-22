#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#define LISTENQ 256
#define MAX_PACKET_LEN (1024*1024)
#define MAX_THREADS 8
#define MAX_TASKS 64

typedef struct {
    void(*function)(void*);
    void *arg;
} task_t;

typedef struct {
    pthread_mutex_t lock;
    pthread_cond_t notify;
    pthread_t *threads;
    task_t *tasks;
    int thread_count;
    int task_count;
    int head;
    int tail;
    int is_shutdown;
    int started_num;
} threadpool_t;

void *thread_func(void *arg) {
    threadpool_t *pool = (threadpool_t *)arg;
    for( ; ; ) {
        pthread_mutex_lock(&(pool->lock));
        while(pool->task_count == 0 && !pool->is_shutdown) {
            pthread_cond_wait(&(pool->notify), &(pool->lock));
        }

        if(pool->is_shutdown) {
            pthread_mutex_unlock(&(pool->lock));
            pthread_exit(NULL);
        }

        task_t task = pool->tasks[pool->head];
        pool->head = (pool->head + 1) % MAX_THREADS;
        pool->task_count--;

        pthread_mutex_unlock(&(pool->lock));

        (*(task.function))(task.arg);
    }
}

void threadpool_init(threadpool_t *pool, int thread_num) {
    int i;

    pthread_mutex_init(&(pool->lock), NULL);
    pthread_cond_init(&(pool->notify), NULL);

    if(thread_num <= 0 || thread_num > MAX_THREADS)
            thread_num = MAX_THREADS;

    pool->threads = (pthread_t *)malloc(sizeof(pthread_t) * thread_num);
    pool->tasks = (task_t *)malloc(sizeof(task_t) * MAX_TASKS);
    pool->thread_count = 0;
    pool->task_count = 0;
    pool->head = 0;
    pool->tail = 0;
    pool->is_shutdown = 0;
    pool->started_num = 0;

    for(i = 0; i < thread_num; i++) {
        pthread_create(&(pool->threads[i]), NULL, thread_func, (void*)pool);
        pool->thread_count++;
        pool->started_num++;
    }
}

void threadpool_destroy(threadpool_t *pool) {
    int i;

    if(pool->is_shutdown)
        return;
    pool->is_shutdown = 1;
    pthread_cond_broadcast(&(pool->notify));

    for(i = 0; i < pool->thread_count; ++i) {
        pthread_join(pool->threads[i], NULL);
    }

    free(pool->threads);
    free(pool->tasks);
    pthread_mutex_destroy(&(pool->lock));
    pthread_cond_destroy(&(pool->notify));
}

int threadpool_add_task(threadpool_t *pool, void(*function)(void *), void *arg) {
    pthread_mutex_lock(&(pool->lock));

    if(pool->task_count == MAX_THREADS) {
        pthread_mutex_unlock(&(pool->lock));
        return -1;
    }
    int tail = pool->tail;
    pool->tasks[tail].function = function;
    pool->tasks[tail].arg = arg;
    pool->tail = (pool->tail + 1) % MAX_THREADS;
    pool->task_count++;

    pthread_cond_signal(&(pool->notify));
    pthread_mutex_unlock(&(pool->lock));

    return 0;
}

typedef struct {
    int fd;
    char *rdbuf;
    int rdbuf_size;
} tcpconn_t;


ssize_t readn(int sockfd, void *vptr, size_t len) {
    size_t nleft;
    ssize_t nread;
    char *p;

    p = (char *)vptr;
    nleft = len;
    while(nleft > 0) {
        if((nread = read(sockfd, p, nleft)) < 0) {
            if(errno = EINTR)
                nread = 0;
            else
                break;
        } else if(nread = 0) {
            break;
        } else {
            nleft -= nread;
            p += nread;
        }
    }
    return (len - nleft);
}

void handle_conn(void *arg) {
    tcpconn_t *tcpconn = (tcpconn_t *)arg;

}


int main(int argc, char **argv) {
    int i, flags, port, connfd, sockfd, maxfdp1, n, connfds[FD_SETSIZE];
    struct sockaddr_in servaddr, cliaddr;
    socklen_t addrlen;
    fd_set readset;
    threadpool_t threadpool;
    char addrbuf[INET_ADDRSTRLEN];

    if(argc != 2) {
        printf("usage tcpserv <port>\n");
        exit(-1);
    }
    port = atoi(argv[1]);
    if(port < 0 || port > (1<<16)-1) {
        printf("port %d must be in [0, 65535]\n", port);
        exit(-1);
    }

    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("socket error:%s\n", strerror(errno));
        exit(-1);
    }

    threadpool_init(&threadpool, 8);

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons((uint16_t)port);

    if(bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        printf("bind error:%s\n", strerror(errno));
        exit(-1);
    }
    if(listen(sockfd, LISTENQ) < 0) {
        printf("listen error:%s\n", strerror(errno));
        exit(-1);
    }

    flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

    FD_ZERO(&readset);
    FD_SET(sockfd, &readset);
    maxfdp1 = sockfd;
    connfds[0] = sockfd;
    for(i = 1; i < FD_SETSIZE; i++)
        connfds[i] = -1;

    for( ; ; ) {
        for(i = 1; i < FD_SETSIZE; i++) {
            if(connfds[i] == -1)
                continue;
            if(connfds[i] > maxfdp1)
                maxfdp1 = connfds[i];
            FD_SET(connfds[i], &readset);
        }
        if((n = select(maxfdp1 + 1, &readset, NULL, NULL, NULL)) < 0) {
            if(errno == EINTR)
                continue;
            else {
                printf("select error: %s\n", strerror(errno));
                break;
            }
        } else {
            if(FD_ISSET(sockfd, &readset)) {
                addrlen = sizeof(cliaddr);
                if((connfd = accept(sockfd, (struct sockaddr*)&cliaddr, &addrlen)) < 0) {
                    printf("accept error:%s\n", strerror(errno));
                } else {
                    if(inet_ntop(AF_INET, &cliaddr.sin_addr, addrbuf, INET_ADDRSTRLEN) == NULL) {
                        printf("inet_ntop error:%s\n", strerror(errno));
                    } else {
                        printf("connect by %s:%d\n", addrbuf, port);
                    }
                    for(i = 1; i < FD_SETSIZE; i++) {
                        if(connfds[i] == -1) {
                            connfds[i] = connfd;
                            break;
                        }
                    }
                }
            }
            for(i = 0; i < FD_SETSIZE; i++) {
                if(FD_ISSET(connfds[i], &readset)) {
                    threadpool_add_task(&threadpool, handle_conn, NULL);
                }
            }
        }
    }
    return 0;
}

