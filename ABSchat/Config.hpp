#pragma once

#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct Config {
    std::string LOG_FILE;
    std::string URL_SERVER_BOT;
    std::string API_KEY;
    std::string FILE_TO_SAVE;
    std::string TG_BOT_KEY;
    std::string SECRET_API_KEY;
    std::string SUPER_SECRET_KEY;
    std::string logs_path;
    std::string minecraft_server_ip;
    int minecraft_server_port;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(Config, LOG_FILE, URL_SERVER_BOT, API_KEY, FILE_TO_SAVE, TG_BOT_KEY, SECRET_API_KEY, SUPER_SECRET_KEY, logs_path, minecraft_server_ip, minecraft_server_port)
};

inline Config loadConfig(const std::string &config_path) {
    std::ifstream config_file(config_path);
    auto config = json::parse(config_file);
    config_file.close();
    return config.template get<Config>();
}
