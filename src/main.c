#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <uv.h>

struct config
{
    char silent;
    char *cache_path;
    int port;
};

static void print_usage(const char *exe);
static void parse_opt(int argc, char *argv[], struct config *conf);
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
parse_opt(int argc, char *argv[], struct config *conf)
{
    int opt;

    conf->silent = 0;
    conf->cache_path = "./";
    conf->port = 8080;

    while ((opt = getopt(argc, argv, "sd:p:h")) != -1)
    {
        switch (opt)
        {
            case 's':
                conf->silent = 1;
                break;
            case 'd':
                conf->cache_path = optarg;
                break;
            case 'p':
                conf->port = atoi(optarg);
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
    buf->base = malloc(suggested_size);
    buf->len = suggested_size;
}

static void
tcp_recv_cb(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf)
{
    printf("%s\n", buf->base);
    free(buf->base);

    uv_close((uv_handle_t*)stream, NULL);
}

static void
new_conn_cb(uv_stream_t *server, int status)
{
    if (status == -1)
    {
        return;
    }

    uv_tcp_t *cl = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
    uv_tcp_init(server->loop, cl);
    if (uv_accept(server, (uv_stream_t*)cl) == 0)
    {
        uv_read_start((uv_stream_t*)cl, alloc_cb, tcp_recv_cb);
    }
}

int
main(int argc, char *argv[])
{
    struct config conf;
    parse_opt(argc, argv, &conf);

    uv_loop_t *loop = uv_default_loop();
    uv_tcp_t server;
    uv_tcp_init(loop, &server);

    struct sockaddr_in bind_addr;
    uv_ip4_addr("127.0.0.1", conf.port, &bind_addr);
    uv_tcp_bind(&server, (const struct sockaddr*)&bind_addr, 0U);
    uv_listen((uv_stream_t*)&server, 128, new_conn_cb);
    return uv_run(loop, UV_RUN_DEFAULT);
}

