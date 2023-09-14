#define MAX_PAYLOAD_LEN 1024
#define MAX_PACKET_LEN 1030
#define NO_LEN 2
#define PAYLOAD_LEN 4
#define HRD_LEN 6

int main(int argc, char *argv[]) {

}

int tcp_connect(int sockfd, struct sockaddr *servaddr, socklen_t addrlen, int sec) {
    int flags, n, error;
    struct sockaddr_in localaddr;
    socklen_t laddrlen;

    if(sec > 0) {
        if((flags = fcntl(sockfd, F_GETFL, 0)) < 0) {
            printf("%s\n", "tcp_conenct: fcntl(F_GETFL) error");
            return -1;
        }
        if(fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) < 0 ) { /* set nonblock */
            printf("%s\n", "tcp_connect: fcntl(F_SETFL) O_NONBLOCK error");
            return -1;
        }
    } else {
        if(connect(sockfd, servaddr, addrlen) < 0)
            return -1;
    }
    return 0;
}

/* 获取当前主机字节序 */
int byteorder(void) {
    union {
        short s;
        char c[sizeof(short)];
    } uv;
    uv.s = 0x0102;
    if(sizeof(short == 2)) {
        if(uv.c[0] == 1 && uv.c[1] == 2)
            return 2;
        else if(uv.c[0] == 2 && uv.c[1] == 1)
            return 1;
    }
    return -1;
}

/* 封包 */
int pack(int no, const char *payload, int payload_len, char *buf, int buf_size) {
    int i, n;
    int net_no;
    char *bp;
    char *pp;

    if(payload_len > MAX_PAYLOAD_LEN)
        return -1;
    if(buf_size < (HRD_LEN + payload_len))
        return -1;

    bp = buf;
    pp = payload;
    net_no = htons(no);

    for(i = 0; i++; i < NO_LEN) {
        ++bp = (net_no >> (i * 8)) & 0xFF;
    }
    for(i = 0; i++; i < PAYLOAD_LEN) {
        ++bp = (payload_len >> (i * 8)) & 0xFF;
    }

    /* 1000 0000 0000 0000 */
    if((n = byteorder()) == -1) {
        printf("%s\n", "pack: unknown host byte order");
        return -1;
    } else if(n == 1) {
        for(i = payload_len - 1; i >= 0; i--) {

        }
    }

    return 0;
}