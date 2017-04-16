#include <iostream>
#include <cstring>
#include "Server.hpp"
#include "HttpRequest.hpp"

using namespace CubaLibre;

const Config *Server::conf;

void
Server::start(const std::string &host, const Config *cf)
{
    uv_loop_t *loop = uv_default_loop();
    conf = cf;

    if (regist_getaddrinfo(loop, accept_getaddr_cb,
                nullptr, host.c_str(), nullptr))
    {
        uv_run(loop, UV_RUN_DEFAULT);
    }
    uv_loop_delete(loop);
}

bool
Server::regist_getaddrinfo(uv_loop_t *loop,
        int (*f)(uv_tcp_t *handle, struct sockaddr *addr),
        void *data, const char *host, const char *port)
{
    struct addrinfo hints;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    getaddr_s *cont = new getaddr_s;
    cont->data = data;
    cont->f = f;

    uv_getaddrinfo_t *getaddrinfo_req = new uv_getaddrinfo_t;
    getaddrinfo_req->data = cont;

    int ret = uv_getaddrinfo(loop, getaddrinfo_req, getaddrinfo_cb,
            host, port, &hints);
    if (ret < 0)
    {
        std::cerr << "uv_getaddrinfo: " << uv_strerror(ret) << std::endl;
        return false;
    }
    return true;
}

void
Server::alloc_buf_cb(uv_handle_t *handle, size_t size, uv_buf_t *buf)
{
    if (!buf->base)
    {
        delete[] buf->base;
    }
    buf->base = new char[size];
    buf->len = size;
}

void
Server::free_tcphandle(uv_handle_t *handle)
{
    delete reinterpret_cast<uv_tcp_t*>(handle);
}

void
Server::getaddrinfo_cb(uv_getaddrinfo_t *req, int status, struct addrinfo *addrs)
{
    unsigned int n = 0U;
    getaddr_s *cont = reinterpret_cast<getaddr_s*>(req->data);

    if (status < 0)
    {
        //pr_err("getaddrinfo(\"%s\"): %s", cf->bind_host, uv_strerror(status));
        goto end;
    }

    {
        unsigned int ipv4_naddrs = 0U;
        unsigned int ipv6_naddrs = 0U;
        for (struct addrinfo *ai = addrs; ai != nullptr; ai = ai->ai_next)
        {
            if (ai->ai_family == AF_INET)
            {
                ipv4_naddrs++;
            }
            else if (ai->ai_family == AF_INET6)
            {
                ipv6_naddrs++;
            }
        }
        if (ipv4_naddrs == 0U && ipv6_naddrs == 0U)
        {
            //pr_err("%s has no IPv4/6 addresses", cf->bind_host);
            goto end;
        }
    }

    for (struct addrinfo *ai = addrs; ai != nullptr; ai = ai->ai_next)
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
        }
        else if (ai->ai_family == AF_INET6)
        {
            s.addr6 = *(const struct sockaddr_in6 *)ai->ai_addr;
            s.addr6.sin6_port = htons(conf->port);
        }
        else
        {
            continue;
        }

        uv_tcp_t *tcp_handle = new uv_tcp_t;
        uv_tcp_init(req->loop, tcp_handle);
        tcp_handle->data = cont->data;

        int err = cont->f(tcp_handle, &s.addr);
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
                n--;
                uv_close((uv_handle_t*)tcp_handle, nullptr);
            }
            break;
        }

        //pr_info("listening on %s:%hu", addrbuf, cf->bind_port);
        n++;
    }

end:
    delete cont;
    delete req;
    uv_freeaddrinfo(addrs);
}

int
Server::accept_getaddr_cb(uv_tcp_t *handle, struct sockaddr *addr)
{
    int err = uv_tcp_bind(handle, addr, 0);
    if (err == 0)
    {
        err = uv_listen(reinterpret_cast<uv_stream_t*>(handle), 128, accept_cb);
    }
    else
    {
        uv_close(reinterpret_cast<uv_handle_t*>(handle), free_tcphandle);
    }
    return err;
}

void
Server::accept_cb(uv_stream_t *server, int status)
{
    uv_tcp_t *cl = nullptr;
    uv_stream_t *scl = nullptr;
    if (status == -1)
    {
        std::cerr << "error on accept_cb" << std::endl;
        goto end;
    }

    cl = new uv_tcp_t;
    scl = reinterpret_cast<uv_stream_t*>(cl);
    uv_tcp_init(uv_default_loop(), cl);
    if (uv_accept(server, scl) == 0)
    {
        uv_read_start(scl, alloc_buf_cb, accept_read_cb);
        return;
    }

end:
    uv_close(reinterpret_cast<uv_handle_t*>(cl), free_tcphandle);
}

void
Server::accept_read_cb(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf)
{
    if (nread < 0)
    {
        delete[] buf->base;
        uv_close(reinterpret_cast<uv_handle_t*>(stream), free_tcphandle);
        return;
    }

    HttpRequest *header = HttpRequest::parseHeader(buf->base);
    header->removeProxyHeaders();
    //std::cerr << header->toString() << std::endl;
    auto host = header->dstHostname();

    regist_getaddrinfo(stream->loop, connect_getaddr_cb,
            header, host.first.c_str(), host.second.c_str());
}

int
Server::connect_getaddr_cb(uv_tcp_t *handle, struct sockaddr *addr)
{
    uv_connect_t *conn_req = new uv_connect_t;
    conn_req->data = handle->data;
    return uv_tcp_connect(conn_req, handle, addr, connect_cb);
}

void
Server::connect_cb(uv_connect_t *req, int status)
{
    HttpRequest *header = static_cast<HttpRequest*>(req->data);
    std::string req_str = header->toString();

    uv_write_t *wreq = new uv_write_t;
    uv_buf_t *buf = new uv_buf_t;
    buf->base = new char[req_str.length()+1];
    buf->len = req_str.length()+1;
    std::memcpy(buf->base, req_str.c_str(), buf->len);

    uv_write(wreq, reinterpret_cast<uv_stream_t*>(req), buf, 1, connect_write_cb);

    delete header;
}

void
Server::connect_write_cb(uv_write_t *req, int status)
{
    delete req;
}

