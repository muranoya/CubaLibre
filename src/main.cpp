#include <cstdio>
#include <unistd.h>
#include "Server.hpp"
#include "Config.hpp"

static void
print_usage(const char *exe)
{
    printf("Usage: %s [-s] [-d path] [-p port] [-h]\n"
           "\t-s\tsilent mode\n"
           "\t-d\tpath to cache directory\n"
           "\t-p\tlisten port\n"
           "\t-h\tshow this help and exit\n",
            exe);
}

static bool
parse_opt(int argc, char *argv[], CubaLibre::Config &conf)
{
    int opt;

    conf.cache_path = "./";
    conf.port = 8080;

    while ((opt = getopt(argc, argv, "d:p:h")) != -1)
    {
        switch (opt)
        {
            case 'd':
                conf.cache_path = optarg;
                break;
            case 'p':
                conf.port = atoi(optarg);
                break;
            case 'h':
                print_usage(argv[0]);
                return false;
                break;
            default:
                print_usage(argv[0]);
                return false;
        }
    }
    return true;
}

int
main(int argc, char *argv[])
{
    CubaLibre::Config conf;
    if (!parse_opt(argc, argv, conf)) return EXIT_FAILURE;
    CubaLibre::Server::start("127.0.0.1", &conf);
    return EXIT_SUCCESS;
}

