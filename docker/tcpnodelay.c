// Shim LD_PRELOAD: liga TCP_NODELAY em todo socket aceito no processo.
// Trantor 1.5.28 expõe setTcpNoDelay mas nunca o chama; com Nagle ligado,
// respostas pequenas sob keep-alive sofrem picos bimodais de ~50ms
// (Nagle + delayed ACK do lado do cliente).
#define _GNU_SOURCE
#include <dlfcn.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>

static void set_nodelay(int fd) {
    if (fd >= 0) {
        int one = 1;
        setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
    }
}

int accept4(int sockfd, struct sockaddr *addr, socklen_t *addrlen, int flags) {
    static int (*real)(int, struct sockaddr *, socklen_t *, int);
    if (!real) {
        real = (int (*)(int, struct sockaddr *, socklen_t *, int))dlsym(RTLD_NEXT, "accept4");
    }
    int fd = real(sockfd, addr, addrlen, flags);
    set_nodelay(fd);
    return fd;
}

int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
    static int (*real)(int, struct sockaddr *, socklen_t *);
    if (!real) {
        real = (int (*)(int, struct sockaddr *, socklen_t *))dlsym(RTLD_NEXT, "accept");
    }
    int fd = real(sockfd, addr, addrlen);
    set_nodelay(fd);
    return fd;
}
