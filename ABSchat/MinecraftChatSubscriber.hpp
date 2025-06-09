#pragma once

#include <cstdlib>
#include <regex>
#include <fstream>
#include <list>

#include <rconpp/rcon.h>

#include "ClientBrocker.hpp"
#include "Service.hpp"

class MinecraftChatSubscriber : public ClientSubscriber
{
public:
    MinecraftChatSubscriber(ClientBrocker& brocker) :
        brocker_{ brocker },
        client_{Service::config.MINECRAFT_SERVER_IP, Service::config.MINECRAFT_SERVER_PORT, Service::config.MINECRAFT_SERVER_PASSWORD }
    {
        brocker_.addSub(this);

        client.start(true);
    }

    void sendMessageToYouself(const ClientMessage& message, const TypeMessage type) override
    {
        std::string command = "/tellraw @a [\"" + message + "\"]";

        client_.send_data(command, 3, rconpp::data_type::SERVERDATA_EXECCOMMAND, [](const rconpp::response& response)
            {
                Service::log.log({ "Response on RCON send message: ", response.data });
            });
    }
    void update() override
    {
        extractMessages();
        for(const auto& i : messages_)
            brocker_.sendMessage(id_, i, TypeMessage::message);
        messages_.clear();
    }


private:
    
    ClientBrocker& brocker_;

    rconpp::rcon_client client_;

    std::list<std::string> messages_;

private:

    void extractMessages()
    {
        static std::streamsize latestLength = 0;
        static bool firstRun = true;
        if (firstRun)
        {
            firstRun = false;

            std::ifstream ifs(Service::config.LATEST_LENGTH_PATH);
            if (ifs.is_open())
            {
                ifs >> latestLength;
                ifs.close();
            }
        }


        std::ifstream logFile(Service::config.LOGS_PATH, std::ios::in | std::ios::binary);
        if (!logFile.is_open())
            return Service::log.log({ "Error: Unable to open log file: ", Service::config.LOGS_PATH });

        logFile.ignore((std::numeric_limits<std::streamsize>::max)());
        std::streamsize length = logFile.gcount();
        logFile.clear();
        logFile.seekg(length < latestLength ? std::streamsize(0) : latestLength, std::ios_base::beg);
        latestLength = length;
        std::ofstream ofs(Service::config.TG_BOT_KEY);
        if (ofs.is_open())
        {
            ofs << latestLength;
            ofs.close();
        }

        std::string line;
        while (getline(logFile, line))
        {
            if (line.find("[Not Secure]") != std::string::npos)
            {
                messages_.push_back((line.substr(line.find("[Not Secure]") + 13)));
            }
        }

        logFile.close();
    }


};
