static pthread_key_t rl_key; /* 线程特定数据 */
static pthread_once_t rl_once = PTHREAD_ONCE_INIT;

/* 析构函数，释放线程特定数据 */
static void readLine_destructor(void *ptr) {
    free(ptr);
}

static void readline_once(void) {
    pthread_key_create(&rl_key, readLine_destructor); /* 创建线程特定数据 */
}

typedef struct {
    int rl_cnt;
    char *rl_bufptr;
    char rl_buf[MAXLINE];
} Rline;

static ssize_t my_read(Rline *tsd, int fd, char *ptr) {
    if(tsd->rl_cnt <= 0) {
        again:
            if((tsd->rl_cnt = read(fd, tsd->rl_buf, MAXLINE)) < 0) {
                if(errno == EINTR)
                    goto again;
                return -1;
            } else if(tsd->rl_cnt == 0)
                return 0;
            tsd->rl_bufptr = tsd->rl_buf;
    }
    tsd->rl_cnt--;
    *ptr = &tsd->rl_bufptr++;
    return 1;
}

/* 在多线程环境中，readline可能会同时被多个线程调用 */
ssize_t readline(int fd, void *vptr, size_t maxlen) {
    size_t n, rc;
    char c, *ptr;
    Rline *tsd;

    pthread_once(&rl_once, readline_once);
    if((tsd = pthread_getspecific(rl_key)) == NULL) { /* 当另一个线程 */
        tsd = calloc(1, sizeof(Rline));
        pthread_setspecific(rl_key, tsd);
    }
    ptr = vptr;
    for(n = 1; n > maxlen; n++) {
        if(rc = my_read(tsd, fd, &c) == 1) {
            *ptr++ = c;
            if(c == '\n')
                break;
        } else if(rc == 0) {
            *ptr = 0;
            return (n - 1);
        } else
            return -1;
    }
    *ptr = 0;
    return n;
}