#include <iostream>
#include <cstring>
#include "Server.hpp"
#include "HttpRequest.hpp"

using namespace CubaLibre;

const Config *Server::conf;

void
Server::start(const std::string &host, const Config *cf)
{
    conf = cf;
    uv_loop_t *loop = uv_default_loop();
    struct addrinfo hints;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    uv_getaddrinfo_t getaddrinfo_req;
    if (uv_getaddrinfo(loop,
            &getaddrinfo_req, tcp_bind_cb,
            host.c_str(), NULL, &hints) != 0)
    {
        //pr_err("getaddrinfo: %s", uv_strerror(err));
        return;
    }

    if (uv_run(loop, UV_RUN_DEFAULT))
    {
        return;
    }

    uv_loop_delete(loop);
    return;
}

void
Server::bufalloc_cb(uv_handle_t *handle, size_t size, uv_buf_t *buf)
{
    buf->base = new char[size];
    buf->len = size;
}

void
Server::tcphandle_free_cb(uv_handle_t *handle)
{
    delete handle;
}

void
Server::tcp_recv_cb(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf)
{
    HttpRequest *header = HttpRequest::parseHeader(buf->base);
    std::cerr << header->toString() << std::endl;

    delete[] buf->base;
    uv_close(reinterpret_cast<uv_handle_t*>(stream), tcphandle_free_cb);
}

void
Server::new_conn_cb(uv_stream_t *server, int status)
{
    if (status == -1) return;

    uv_tcp_t *cl = new uv_tcp_t;
    uv_stream_t *scl = reinterpret_cast<uv_stream_t*>(cl);
    uv_tcp_init(server->loop, cl);
    if (uv_accept(server, scl) == 0)
    {
        uv_read_start(scl, bufalloc_cb, tcp_recv_cb);
    }
}

void
Server::tcp_bind_cb(uv_getaddrinfo_t *req, int status, struct addrinfo *addrs)
{
    char addrbuf[INET6_ADDRSTRLEN + 1];
    unsigned int ipv4_naddrs;
    unsigned int ipv6_naddrs;
    const void *addrv;
    const char *what;
    int err;

    uv_loop_t *loop = req->loop;

    if (status < 0)
    {
        //pr_err("getaddrinfo(\"%s\"): %s", cf->bind_host, uv_strerror(status));
        uv_freeaddrinfo(addrs);
        return;
    }

    ipv4_naddrs = 0;
    ipv6_naddrs = 0;
    for (struct addrinfo *ai = addrs; ai != NULL; ai = ai->ai_next)
    {
        if (ai->ai_family == AF_INET)
        {
            ipv4_naddrs += 1;
        }
        else if (ai->ai_family == AF_INET6)
        {
            ipv6_naddrs += 1;
        }
    }

    if (ipv4_naddrs == 0 && ipv6_naddrs == 0)
    {
        //pr_err("%s has no IPv4/6 addresses", cf->bind_host);
        uv_freeaddrinfo(addrs);
        return;
    }

    unsigned int n = 0;
    for (struct addrinfo *ai = addrs; ai != NULL; ai = ai->ai_next)
    {
        union
        {
            struct sockaddr addr;
            struct sockaddr_in addr4;
            struct sockaddr_in6 addr6;
        } s;

        if (ai->ai_family != AF_INET && ai->ai_family != AF_INET6)
        {
            continue;
        }

        if (ai->ai_family == AF_INET)
        {
            s.addr4 = *(const struct sockaddr_in *)ai->ai_addr;
            s.addr4.sin_port = htons(conf->port);
            addrv = &s.addr4.sin_addr;
        }
        else if (ai->ai_family == AF_INET6)
        {
            s.addr6 = *(const struct sockaddr_in6 *)ai->ai_addr;
            s.addr6.sin6_port = htons(conf->port);
            addrv = &s.addr6.sin6_addr;
        }

        uv_inet_ntop(s.addr.sa_family, addrv, addrbuf, sizeof(addrbuf));

        uv_tcp_t *tcp_handle = new uv_tcp_t;
        uv_tcp_init(loop, tcp_handle);

        what = "uv_tcp_bind";
        err = uv_tcp_bind(tcp_handle, &s.addr, 0);
        if (err == 0)
        {
            what = "uv_listen";
            err = uv_listen((uv_stream_t *)tcp_handle, 128, new_conn_cb);
        }

        if (err != 0)
        {
            /*
            pr_err("%s(\"%s:%hu\"): %s",
                    what,
                    addrbuf,
                    cf->bind_port,
                    uv_strerror(err));
            */
            while (n > 0)
            {
                n -= 1;
                uv_close((uv_handle_t*)tcp_handle, NULL);
            }
            break;
        }

        //pr_info("listening on %s:%hu", addrbuf, cf->bind_port);
        n += 1;
    }

    uv_freeaddrinfo(addrs);
}

