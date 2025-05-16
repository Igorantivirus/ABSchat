#pragma once

#include <map>

#include <tgbot/tgbot.h>
#include <nlohmann/json.hpp>
#include <sio_client.h>
#include <jwt-cpp/jwt.h>

#include "HTTPClient.hpp"
#include "HardCode.hpp"

class Bot
{
public:
    Bot(const std::string key) :
        bot(key)
    {
        initResponsesTG();
    }

    void Run()
    {
        try
        {
            std::cout << "Bot username: " << bot.getApi().getMe()->username.c_str() << '\n';
            TgBot::TgLongPoll longPoll(bot);
            while (true)
            {
                std::cout << "Long poll started" << '\n';
                longPoll.start();
            }
        }
        catch (const TgBot::TgException& e)
        {
            std::cout << "Bot error: " << e.what() << '\n';
        }
        catch (...)
        {
            std::cout << "Unknown Error" << '\n';
        }
    }

private:

	TgBot::Bot bot;
    //первое id - с тг, второе с сайта
    std::map<std::string, std::string> users;

    

private:

    std::string getRequest(const std::string& authCode, std::string userId)
    {
        std::map<std::string, std::string> json = {
            {"code_server", "o48c9qw0m4"},
            {"act", "reg"},
            {"code", authCode},
            {"id_tg", userId},
        };

        HttpClient client;

        std::string result = client.getRequastParam("http://beta.abserver.ru/bots_act/tg", json);

        if (result.empty())
        {
            std::cerr << "Error of get request\n";
            return "";
        }

        nlohmann::json res = nlohmann::json::parse(result);

        if (res["code"] != "true")
        {
            std::cerr << "Error of connect" << '\n';
            return "";
        }

        return res["id"];
    }

    bool registered(const std::string& idTg)
    {
        auto found = users.find(idTg);
        return found != users.end();
    }

    void sendMessageToServer(const std::string& str)
    {

    }

	void initResponsesTG()
	{
        bot.getEvents().onCommand("start", [this](TgBot::Message::Ptr message)
            {
                std::string mes = message->text;
                if (std::size_t ind = mes.find(' '); ind != std::string::npos)
                    mes.erase(mes.begin(), mes.begin() + ind + 1);

                std::string userId = std::to_string(message->from->id);

                std::string idTg = getRequest(mes, userId);

                if (idTg.empty())
                {
                    bot.getApi().sendMessage(message->chat->id, to_utf8(L"Успешная регистрация пользователя!"));
                }
                else
                {
                    bot.getApi().sendMessage(message->chat->id, to_utf8(L"Пошёл вон - шалопай!"));
                    users[idTg] = userId;
                }
            });

        

        bot.getEvents().onAnyMessage([this](TgBot::Message::Ptr message)
            {
                if (message->text.empty() || message->text[0] == '/')
                    return;

                if (!registered(std::to_string(message->from->id)))
                    bot.getApi().sendMessage(message->chat->id, to_utf8(L"Сообщение не будет отправлено! Вы не серверный чел!"));
                else
                    sendMessageToServer(message->text);

            });

	}

    

};