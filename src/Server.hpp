#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <uv.h>
#include "Uncopyable.hpp"
#include "Config.hpp"

namespace CubaLibre
{

class Server : private Uncopyable
{
public:
    static void start(const std::string &host, const Config *conf);

private:
    static const Config *conf;
    Server() = delete;
    ~Server() = delete;

    struct getaddr_s
    {
        void *data;
        int (*f)(uv_tcp_t *handle, struct sockaddr *addr);
    };

    static bool regist_getaddrinfo(uv_loop_t *loop,
            int (*f)(uv_tcp_t *handle, struct sockaddr *addr),
            void *data, const char *host, const char *port);
    static void alloc_buf_cb(uv_handle_t *handle, size_t size, uv_buf_t *buf);
    static void free_tcphandle(uv_handle_t *handle);
    static void getaddrinfo_cb(uv_getaddrinfo_t *req, int status, struct addrinfo *addrs);

    /* income callbacks */
    static int accept_getaddr_cb(uv_tcp_t *handle, struct sockaddr *addr);
    static void accept_cb(uv_stream_t *server, int status);
    static void accept_read_cb(uv_stream_t *stream, ssize_t readsize, const uv_buf_t *buf);

    /* outcome callbacks */
    static int connect_getaddr_cb(uv_tcp_t *handle, struct sockaddr *addr);
    static void connect_cb(uv_connect_t *req, int status);
    static void connect_write_cb(uv_write_t *req, int status);
};

};

#endif /* SERVER_HPP */
