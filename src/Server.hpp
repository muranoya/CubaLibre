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

    /* callbacks */
    static void bufalloc_cb(uv_handle_t *handle, size_t size, uv_buf_t *buf);
    static void tcphandle_free_cb(uv_handle_t *handle);
    static void tcp_recv_cb(uv_stream_t *stream, ssize_t readsize, const uv_buf_t *buf);
    static void new_conn_cb(uv_stream_t *server, int status);
    static void tcp_bind_cb(uv_getaddrinfo_t *req, int status, struct addrinfo *addrs);
};

};

#endif /* SERVER_HPP */
