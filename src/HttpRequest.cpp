#include <cstring>
#include <memory>
#include <iostream>
#include "HttpRequest.hpp"

using namespace CubaLibre;

HttpRequest::HttpRequest()
    : method(HTTP_INVALID)
    , req_uri("")
    , version(HTTPV_INVALID)
    , headers()
{
}

HttpRequest::~HttpRequest()
{
}

std::string
HttpRequest::toString()
{
    const std::string http_method_str[] = {
        "Invalid HTTP method",
        "GET",
        "POST",
        "PUT",
        "HEAD",
        "OPTION",
        "DELETE",
        "TRACE",
        "CONNECT",
    };
    const std::string http_ver_str[] = {
        "Invalid HTTP version",
        "HTTP/1.1",
        "HTTP/1.0",
        "HTTP/0.9",
    };

    std::string str;
    str = http_method_str[method] + " " +
        req_uri + " " +
        http_ver_str[version] + "\r\n";
    for (auto &i : headers)
    {
        str += i.first + ":" + i.second + "\r\n";
    }
    return str;
}

void
HttpRequest::removeProxyHeaders()
{
    headers.erase("Proxy-Connection");
}

std::pair<std::string, std::string>
HttpRequest::dstHostname() const
{
    const std::string hostname = headers.at("Host");
    auto pos = hostname.find(':');
    std::string host = hostname.substr(0, pos);
    std::string port = "80";
    if (pos != std::string::npos)
    {
        port = hostname.substr(pos+1, std::string::npos).c_str();
    }

    return std::make_pair(host, port);
}

HttpRequest *
HttpRequest::parseHeader(const char *str)
{
    std::unique_ptr<HttpRequest> h(new HttpRequest());

    auto vec = split(std::string(str), "\r\n");
    auto iter = vec.cbegin();
    if (!parseRequestLine(*iter++, *h.get())) return nullptr;

    for (; iter != vec.cend(); ++iter)
    {
        auto f = split(*iter, ":", 2);
        if (f.size() != 2) break;
        h->headers.insert(std::make_pair(f[0], trim(f[1])));
    }

    return h.release();
}

std::vector<std::string>
HttpRequest::split(const std::string &str, const std::string &dlm, int n)
{
    int b, i;
    std::vector<std::string> vec;
    for (b = 0, i = 1; i != n; i++)
    {
        auto e = str.find(dlm, b);
        vec.push_back(std::string(str, b, e-b));
        if (e == std::string::npos) return vec;
        b = e + dlm.length();
    }
    vec.push_back(std::string(str, b, str.length()-b));
    return vec;
}

const char *
HttpRequest::skip(const char *str)
{
    while (*str != '\0' && *str != '\r'&& *str != '\n' &&
            *str == ' ') ++str;
    return str;
}

int
HttpRequest::nextWhiteSpace(const char *str)
{
    const char *p = str;
    while (*p != '\0' && *p != '\r'&& *p != '\n' &&
            *p != ' ') ++p;
    return p - str;
}

std::string
HttpRequest::trim(const std::string &str)
{
    int s, e;
    const int len = str.length();
    for (s = 0; s < len; ++s)
    {
        if (str[s] != ' ') break;
    }
    for (e = 0; e < len; ++e)
    {
        if (str[len-e-1] != ' ') break;
    }

    return str.substr(s, len-s-e);
}

std::string
HttpRequest::convToBrowserURI(const std::string &uri)
{
    const std::string http_s("http://");
    auto pos = uri.find(http_s);
    if (pos == 0)
    {
        auto pos2 = uri.find('/', http_s.length());
        if (pos2 == std::string::npos)
        {
            return "/";
        }
        return uri.substr(pos2, std::string::npos);
    }
    
    const std::string https_s("https://");
    pos = uri.find(https_s);
    if (pos == 0)
    {
        auto pos2 = uri.find('/', https_s.length());
        if (pos2 == std::string::npos)
        {
            return "/";
        }
        return uri.substr(pos2, std::string::npos);
    }

    return uri;
}

bool
HttpRequest::parseRequestLine(const std::string &str, HttpRequest &h)
{
    const char *p = str.data();

    /* HTTP command */
    p = skip(p);
    if (p == '\0')
    {
        return false;
    }
    else
    {
        const std::pair<std::string, http_method> methods[] = {
            {"GET",     HTTP_GET},
            {"POST",    HTTP_POST},
            {"PUT",     HTTP_PUT},
            {"HEAD",    HTTP_HEAD},
            {"OPTION",  HTTP_OPTION},
            {"DELETE",  HTTP_DELETE},
            {"TRACE",   HTTP_TRACE},
            {"CONNECT", HTTP_CONNECT},
        };
        const int len = nextWhiteSpace(p);
        const std::string cmd(p, len);
        p += len;
        for (const auto &c : methods)
        {
            if (c.first == cmd)
            {
                h.method = c.second;
                break;
            }
        }
        if (h.method == HTTP_INVALID) return false;
    }

    /* HTTP request uri */
    p = skip(p);
    if (p == '\0')
    {
        return false;
    }
    else
    {
        const int len = nextWhiteSpace(p);
        h.req_uri = std::string(p, len);
        p += len;
        if (h.req_uri.empty()) return false;
    }

    /* HTTP version */
    p = skip(p);
    if (p == '\0')
    {
        h.version = HTTPV_0_9;
    }
    else
    {
        const int len = nextWhiteSpace(p);
        const std::string ver(p, len);
        if (ver == "HTTP/1.1")
        {
            h.version = HTTPV_1_1;
        }
        else if (ver == "HTTP/1.0")
        {
            h.version = HTTPV_1_0;
        }
        else
        {
            return false;
        }
    }

    return true;
}

