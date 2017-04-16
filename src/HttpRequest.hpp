#ifndef HTTPHEADER_HPP
#define HTTPHEADER_HPP

#include <string>
#include <unordered_map>
#include <vector>

namespace CubaLibre
{

class HttpRequest
{
public:
    enum http_method
    {
        HTTP_INVALID,
        HTTP_GET,
        HTTP_POST,
        HTTP_PUT,
        HTTP_HEAD,
        HTTP_OPTION,
        HTTP_DELETE,
        HTTP_TRACE,
        HTTP_CONNECT,
    };
    enum http_version
    {
        HTTPV_INVALID,
        HTTPV_1_1,
        HTTPV_1_0,
        HTTPV_0_9,
    };

    static HttpRequest *parseHeader(const char *str);
    ~HttpRequest();

    std::string toString();

    void removeProxyHeaders();

    std::pair<std::string, std::string> dstHostname() const;

private:
    explicit HttpRequest();

    http_method method;
    std::string req_uri;
    http_version version;
    std::unordered_map<std::string, std::string> headers;

    static std::vector<std::string> split(
            const std::string &str, const std::string &dlm, int n = -1);
    static const char *skip(const char *str);
    static int nextWhiteSpace(const char *str);
    static std::string trim(const std::string &str);
    static std::string convToBrowserURI(const std::string &uri);

    static bool parseRequestLine(const std::string &str, HttpRequest &h);
};

}

#endif /* HTTPHEADER_HPP */
