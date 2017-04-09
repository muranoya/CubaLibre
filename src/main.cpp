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

