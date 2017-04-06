#include <unistd.h>
#include <iostream>
#include <uv.h>
#include "config.hpp"

static void
print_usage(const char *exe)
{
    std::cout <<
        "Usage: " << exe
        << std::endl;
    exit(0);
}

static void
parse_opt(int argc, char *argv[])
{
    int opt;
    while ((opt = getopt(argc, argv, "d:p:h")) != -1)
    {
        switch (opt)
        {
            case 'd':
                Config::cache_path = optarg;
                break;
            case 'p':
                Config::port = std::atoi(optarg);
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
    buf->base = new char[suggested_size];
    buf->len = suggested_size;
}

static void
tcp_recv_cb(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf)
{
    std::cout << buf->base << std::endl;
    delete[] buf->base;
}

static void
new_conn_cb(uv_stream_t *server, int status)
{
    if (status == -1)
    {
        return;
    }

    uv_tcp_t *cl = new uv_tcp_t;
    uv_tcp_init(server->loop, cl);
    if (uv_accept(server, (uv_stream_t*)cl) == 0)
    {
        uv_read_start((uv_stream_t*)cl, alloc_cb, tcp_recv_cb);
    }

    delete cl;
}

int
main(int argc, char *argv[])
{
    parse_opt(argc, argv);

    uv_loop_t *loop = uv_default_loop();
    uv_tcp_t server;
    uv_tcp_init(loop, &server);

    struct sockaddr_in bind_addr;
    uv_ip4_addr("127.0.0.1", 8080, &bind_addr);
    uv_tcp_bind(&server, (const struct sockaddr*)&bind_addr, 0U);
    uv_listen((uv_stream_t*)&server, 128, new_conn_cb);
    return uv_run(loop, UV_RUN_DEFAULT);
}

