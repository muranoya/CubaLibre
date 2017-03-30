#include <unistd.h>
#include <iostream>
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

int
main(int argc, char *argv[])
{
    parse_opt(argc, argv);
    return 0;
}

