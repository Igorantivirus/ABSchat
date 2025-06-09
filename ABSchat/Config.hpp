#pragma once

#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct Config
{
    std::string LOG_FILE;
    std::string CHATS_FILE;

    std::string TG_BOT_KEY;

    std::string LOGS_PATH;
    std::string MINECRAFT_SERVER_IP;
    int MINECRAFT_SERVER_PORT;
    std::string MINECRAFT_SERVER_PASSWORD;
    
    std::string LATEST_LENGTH_PATH;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(Config, LOG_FILE, CHATS_FILE, TG_BOT_KEY, LOGS_PATH, MINECRAFT_SERVER_IP, MINECRAFT_SERVER_PORT, MINECRAFT_SERVER_PASSWORD, LATEST_LENGTH_PATH)
};

inline Config loadConfig(const std::string &config_path)
{
    std::ifstream config_file(config_path);
    auto config = json::parse(config_file);
    config_file.close();
    return config.template get<Config>();
}
