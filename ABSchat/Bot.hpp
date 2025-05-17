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
        initListeningSock();

        connectWithServer();
    }

    void Run()
    {
        try
        {
            std::cout << "Bot username: " << bot.getApi().getMe()->username.c_str() << '\n';
            TgBot::TgLongPoll longPoll(bot);
            while (true)
            {
                if (!client.opened())
                {
                    std::cout << "Connection faled";
                    break;
                }
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
    sio::client client;
    //первое id - с тг, второе с сайта
    std::map<std::string, std::string> users;

private:

    void connectWithServer()
    {
        std::string url = "https://beta/abserver.ru";

        std::string token = jwt::create()
            .set_payload_claim("connect_by", jwt::claim(std::string("tg")))
            .set_payload_claim("api_pass", jwt::claim(std::string("o48c9qw0m4")))
            .sign(jwt::algorithm::hs256{ SUPER_SECRET_KEY });

        std::map<std::string, std::string> query;
        query["token"] = token;
        client.connect("http://beta.abserver.ru", query);
    }

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

    void initListeningSock()
    {
        client.set_open_listener([&]()
            {
                std::cout << "Connect sucsess" << std::endl;
            });

        client.set_fail_listener([&]() {
            std::cout << "Error of connect" << std::endl;
            });

        client.set_close_listener([](sio::client::close_reason const& reason) {
            std::cout << "Connect closed. Reaon: " << static_cast<int>(reason) << std::endl;
            });

        client.socket()->on("message", [](sio::event& ev)
            {
                /*sio::message::ptr data = ev.get_message();

                if (data->get_flag() == sio::message::flag_object) {
                    auto obj = data->get_map();

                    auto userIt = obj.find("user");
                    auto textIt = obj.find("text");

                    std::string user = (userIt != obj.end() && userIt->second) ? userIt->second->get_string() : "???";
                    std::string text = (textIt != obj.end() && textIt->second) ? textIt->second->get_string() : "";

                    std::cout << "[" << user << "] " << text << std::endl;
                }
                else {
                    std::cout << "Get message, but its not a object" << std::endl;
                }*/
            });
    }

};