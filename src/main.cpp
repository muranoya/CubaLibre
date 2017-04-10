#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <string>
#include <iostream>
#include <uv.h>
#include "httpheader.hpp"

namespace CubaLibre
{

struct Config
{
    char silent;
    std::string cache_path;
    int port;
};

static void print_usage(const char *exe);
static void parse_opt(int argc, char *argv[], Config &conf);
static void alloc_cb(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf);
static void tcp_recv_cb(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf);
static void new_conn_cb(uv_stream_t *server, int status);

static void
print_usage(const char *exe)
{
    printf("Usage: %s [-s] [-d path] [-p port] [-h]\n"
           "\t-s\tsilent mode\n"
           "\t-d\tpath to cache directory\n"
           "\t-p\tlisten port\n"
           "\t-h\tshow this help and exit\n",
            exe);
    exit(0);
}

static void
parse_opt(int argc, char *argv[], Config &conf)
{
    int opt;

    conf.silent = 0;
    conf.cache_path = "./";
    conf.port = 8080;

    while ((opt = getopt(argc, argv, "sd:p:h")) != -1)
    {
        switch (opt)
        {
            case 's':
                conf.silent = 1;
                break;
            case 'd':
                conf.cache_path = optarg;
                break;
            case 'p':
                conf.port = atoi(optarg);
                break;
            case 'h':
                print_usage(argv[0]);
                break;
            default:
                print_usage(argv[0]);
        }
    }
}

static void
alloc_cb(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf)
{
    buf->base = static_cast<char*>(std::malloc(suggested_size));
    buf->len = suggested_size;
}

static void
tcp_recv_cb(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf)
{
    //printf("%s\n", buf->base);

    HttpHeader *header = HttpHeader::parseHeader(buf->base);
    std::cerr << header->toString() << std::endl;
    //uv_tcp_t *sock = static_cast<uv_tcp_t*>(std::malloc(sizeof(uv_tcp_t)));
    //uv_tcp_init(stream->loop, sock);
    //uv_connect_t *conn = static_cast<uv_connect_t*>(std::malloc(sizeof(uv_connect_t)));

    free(buf->base);
    uv_close(reinterpret_cast<uv_handle_t*>(stream), NULL);
}

static void
new_conn_cb(uv_stream_t *server, int status)
{
    if (status == -1) return;

    uv_tcp_t *cl = static_cast<uv_tcp_t*>(std::malloc(sizeof(uv_tcp_t)));
    uv_stream_t *scl = reinterpret_cast<uv_stream_t*>(cl);
    uv_tcp_init(server->loop, cl);
    if (uv_accept(server, scl) == 0)
    {
        uv_read_start(scl, alloc_cb, tcp_recv_cb);
    }
}

static void
do_bind(uv_getaddrinfo_t *req, int status, struct addrinfo *addrs)
{
    char addrbuf[INET6_ADDRSTRLEN + 1];
    unsigned int ipv4_naddrs;
    unsigned int ipv6_naddrs;
    server_state *state;
    server_config *cf;
    struct addrinfo *ai;
    const void *addrv;
    const char *what;
    uv_loop_t *loop;
    server_ctx *sx;
    unsigned int n;
    int err;
    union {
        struct sockaddr addr;
        struct sockaddr_in addr4;
        struct sockaddr_in6 addr6;
    } s;

    state = CONTAINER_OF(req, server_state, getaddrinfo_req);
    loop = state->loop;
    cf = &state->config;

    if (status < 0) {
        pr_err("getaddrinfo(\"%s\"): %s", cf->bind_host, uv_strerror(status));
        uv_freeaddrinfo(addrs);
        return;
    }

    ipv4_naddrs = 0;
    ipv6_naddrs = 0;
    for (ai = addrs; ai != NULL; ai = ai->ai_next) {
        if (ai->ai_family == AF_INET) {
            ipv4_naddrs += 1;
        } else if (ai->ai_family == AF_INET6) {
            ipv6_naddrs += 1;
        }
    }

    if (ipv4_naddrs == 0 && ipv6_naddrs == 0) {
        pr_err("%s has no IPv4/6 addresses", cf->bind_host);
        uv_freeaddrinfo(addrs);
        return;
    }

    state->servers =
        xmalloc((ipv4_naddrs + ipv6_naddrs) * sizeof(state->servers[0]));

    n = 0;
    for (ai = addrs; ai != NULL; ai = ai->ai_next) {
        if (ai->ai_family != AF_INET && ai->ai_family != AF_INET6) {
            continue;
        }

        if (ai->ai_family == AF_INET) {
            s.addr4 = *(const struct sockaddr_in *) ai->ai_addr;
            s.addr4.sin_port = htons(cf->bind_port);
            addrv = &s.addr4.sin_addr;
        } else if (ai->ai_family == AF_INET6) {
            s.addr6 = *(const struct sockaddr_in6 *) ai->ai_addr;
            s.addr6.sin6_port = htons(cf->bind_port);
            addrv = &s.addr6.sin6_addr;
        } else {
            UNREACHABLE();
        }

        if (uv_inet_ntop(s.addr.sa_family, addrv, addrbuf, sizeof(addrbuf))) {
            UNREACHABLE();
        }

        sx = state->servers + n;
        sx->loop = loop;
        sx->idle_timeout = state->config.idle_timeout;
        CHECK(0 == uv_tcp_init(loop, &sx->tcp_handle));

        what = "uv_tcp_bind";
        err = uv_tcp_bind(&sx->tcp_handle, &s.addr, 0);
        if (err == 0) {
            what = "uv_listen";
            err = uv_listen((uv_stream_t *) &sx->tcp_handle, 128, on_connection);
        }

        if (err != 0) {
            pr_err("%s(\"%s:%hu\"): %s",
                    what,
                    addrbuf,
                    cf->bind_port,
                    uv_strerror(err));
            while (n > 0) {
                n -= 1;
                uv_close((uv_handle_t *) (state->servers + n), NULL);
            }
            break;
        }

        pr_info("listening on %s:%hu", addrbuf, cf->bind_port);
        n += 1;
    }

    uv_freeaddrinfo(addrs);
}


static void
startserver(const char *host, uv_loop_t *loop)
{
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    uv_getaddrinfo_t getaddrinfo_req;
    if (uv_getaddrinfo(loop,
            &getaddrinfo_req, do_bind,
            host, NULL, &hints) != 0)
    {
        //pr_err("getaddrinfo: %s", uv_strerror(err));
        return;
    }

    if (uv_run(loop, UV_RUN_DEFAULT))
    {
        return;
    }

    uv_loop_delete(loop);
    free(state.servers);
    return;
}

};

int
main(int argc, char *argv[])
{
    CubaLibre::Config conf;
    CubaLibre::parse_opt(argc, argv, conf);

    uv_loop_t *loop = uv_default_loop();
    uv_tcp_t server;
    uv_tcp_init(loop, &server);

    struct sockaddr_in bind_addr;
    uv_ip4_addr("127.0.0.1", conf.port, &bind_addr);
    uv_tcp_bind(&server, reinterpret_cast<const struct sockaddr*>(&bind_addr), 0U);
    uv_listen(reinterpret_cast<uv_stream_t*>(&server), 128, CubaLibre::new_conn_cb);
    return uv_run(loop, UV_RUN_DEFAULT);
}

