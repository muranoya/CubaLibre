#ifndef CONFIG_HPP
#define CONFIG_HPP

class Config
{
public:
    static int port;
    static const char *cache_path;

    static void Save();
    static void Load();

private:
    Config() = delete;

};

#endif // CONFIG_HPP
