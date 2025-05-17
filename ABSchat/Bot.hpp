#pragma once

#include <map>
#include <set>
#include <chrono>
#include <thread>

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
                    reconectTry();
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
    std::set<int64_t> chats;

private:

    void reconectTry()
    {
        std::chrono::milliseconds timespan(5000); // 1 second
        while (!client.opened())
        {
            std::cout << "Try to connect ";
            connectWithServer();
            std::this_thread::sleep_for(timespan);
        }
    }

    //подключение к серверу
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

    //Регистрация пользователя на сервере
    //return id пользователя на сайте
    std::string getRequest(const std::string& authCode, std::string userId)
    {
        std::map<std::string, std::string> json =
        {
            {"code_server", "o48c9qw0m4"},
            {"act", "reg"},
            {"code", authCode},
            {"id_tg", userId},
        };

        HttpClient client;
        std::string result = client.getRequastParam("http://beta.abserver.ru/bots_act/tg", json);

        if (result.empty())
        {
            std::cout << "Error of get request\n";
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

    //проверка что пользователь зареган (idTg)
    bool registered(const std::string& idTg)
    {
        auto found = users.find(idTg);
        return found != users.end();
    }

    void sendMessageToServer(const std::string& messageToServer, const std::string& userIds)
    {
        if (messageToServer.empty())
            return;

        auto msg = sio::object_message::create();
        msg->get_map()["message"] = sio::string_message::create(messageToServer);
        msg->get_map()["id"] = sio::string_message::create(userIds);

        client.socket()->emit("message_tg", sio::string_message::create(messageToServer));
    }

    void sendMessageToAllTgExcept(const std::string& message, const std::int64_t exceptIdTg = 0)
    {
        for (const auto& chatId : chats)
            if(chatId != exceptIdTg)
                bot.getApi().sendMessage(chatId, message);
    }

#pragma region BotResponser

    void start(TgBot::Message::Ptr message)
    {
        std::string mes = message->text;
        if (std::size_t ind = mes.find(' '); ind != std::string::npos)
            mes.erase(mes.begin(), mes.begin() + ind + 1);

        std::string userId = std::to_string(message->from->id);
        std::string idTg = getRequest(mes, userId);

        if (idTg.empty())
            bot.getApi().sendMessage(message->chat->id, to_utf8(L"Пошёл вон - шалопай!"));
        else
        {
            bot.getApi().sendMessage(message->chat->id, to_utf8(L"Успешная регистрация пользователя!"));
            users[idTg] = userId;
            chats.insert(message->chat->id);
        }
    }
    void processMessage(TgBot::Message::Ptr message)
    {
        if (message->text.empty() || message->text[0] == '/')
            return;

        if (!registered(std::to_string(message->from->id)))
            bot.getApi().sendMessage(message->chat->id, to_utf8(L"Сообщение не будет отправлено! Вы не серверный чел!"));
        else
        {
            sendMessageToServer(message->text, users[std::to_string(message->chat->id)]);
            std::string mes = '<' + message->from->username + '>' + message->text;
            sendMessageToAllTgExcept(mes, message->chat->id);
        }
    }

	void initResponsesTG()
	{
        bot.getEvents().onCommand("start", [this](TgBot::Message::Ptr message) {start(message); });
        bot.getEvents().onAnyMessage([this](TgBot::Message::Ptr message) {processMessage(message); });
	}

#pragma endregion

#pragma region clientListener

    void connectSuccess()
    {
        std::cout << "Connect sucsess" << std::endl;
    }
    void connectFatal()
    {
        std::cout << "Connect fatal" << std::endl;
    }
    void connectClosed(sio::client::close_reason const& reason)
    {
        std::cout << "Connect closed. Reaon: " << static_cast<int>(reason) << std::endl;
    }
    void onMessageResponse(sio::event& ev)
    {
        sio::message::ptr data = ev.get_message();

        if (data->get_flag() != sio::message::flag_object)
        {
            std::cout << "Get message, but its not a object" << std::endl;
            return;
        }
        auto obj = data->get_map();

        auto userIt = obj.find("user");
        auto messageIt = obj.find("message");
        auto typeIt = obj.find("type");

        std::string user = (userIt != obj.end() && userIt->second) ? userIt->second->get_string() : "???";
        std::string message = (messageIt != obj.end() && messageIt->second) ? messageIt->second->get_string() : "";
        std::string type = (typeIt != obj.end() && typeIt->second) ? typeIt->second->get_string() : "";

        sendMessageToAllTgExcept('<' + user + '>' + message);
    }

    void initListeningSock()
    {
        client.set_open_listener([this]() {connectSuccess(); });
        client.set_fail_listener([this]() {connectFatal(); });
        client.set_close_listener([this](sio::client::close_reason const& reason) {connectClosed(reason); });
        client.socket()->on("message", [this](sio::event& ev) {onMessageResponse(ev); });
    }

#pragma endregion


};