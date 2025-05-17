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
#include "FileSaver.hpp"
#include "LogOutput.hpp"

#define URL_SERVER_BOT "http://beta.abserver.ru/bots_act/tg"
#define API_KEY "o48c9qw0m4"
#define FILE_TO_SAVE "UsersInfo.txt"
#define LOG_FILE "Log.txt"

using RowJson = std::map<std::string, std::string>;

class Bot
{
public:
    Bot(const std::string key) :
        bot(key), saver{ users, chats, FILE_TO_SAVE }, log{ LOG_FILE, true}
    {
        saver.readFromFile();

        initResponsesTG();
        initListeningSock();

        connectWithServer();
    }
    ~Bot()
    {
        saver.saveToFile();
    }

    void run()
    {
        try
        {
            log.log({ "Bot started. Username: ", bot.getApi().getMe()->username.c_str() });
            TgBot::TgLongPoll longPoll(bot);
            while (true)
            {
                if (!client.opened())
                    reconectTry();
                log.log("Long poll started");
                longPoll.start();
            }
        }
        catch (const TgBot::TgException& e)
        {
            log.log({ "Bot error: ",e.what() });
        }
        catch (const std::exception& e)
        {
            log.log({ "Std error: ",e.what() });
        }
        catch (...)
        {
            log.log("Unknown Error");
        }
    }
    
private:

	TgBot::Bot bot;
    sio::client client;
    //первое id - с тг, второе с сайта
    std::map<std::string, std::string> users;
    std::set<int64_t> chats;

    FileSaver saver;
    LogOutput log;

private:

    std::string getUserIdFromServSafely(const std::int64_t id)
    {
        std::string idTg = std::to_string(id);
        auto found = users.find(idTg);
        if (found == users.end())
            return "";
        return found->second;
    }

    void reconectTry()
    {
        log.log("Connectfailed, next try to reconnecd");
        std::chrono::milliseconds timespan(5000); // 1 second
        while (!client.opened())
        {
            connectWithServer();
            std::this_thread::sleep_for(timespan);
        }
    }

    //подключение к серверу
    void connectWithServer()
    {
        log.log("Connect try");
        std::string url = "http://beta.abserver.ru:5050";

        std::string token = jwt::create()
            .set_payload_claim("connect_by", jwt::claim(std::string("tg")))
            .set_payload_claim("api_pass", jwt::claim(std::string("o48c9qw0m4")))
            .sign(jwt::algorithm::hs256{ SUPER_SECRET_KEY });

        std::map<std::string, std::string> query;
        query["token"] = token;
        client.connect(url, query);
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

        client.socket()->emit("message_tg", msg);
    }

    void sendMessageToAllTgExcept(const std::string& message, const std::int64_t exceptIdTg = 0)
    {
        for (const auto& chatId : chats)
            if(chatId != exceptIdTg)
                bot.getApi().sendMessage(chatId, message);
    }

    #pragma region botResponser

    void start(TgBot::Message::Ptr message)
    {
        std::string pr = message->text;
        RowJson param =
        {
            {"code_server", "o48c9qw0m4"},
            {"act", "reg"},
            {"code", eraseBeforeForstSpace(message->text)},
            {"id_tg", std::to_string(message->from->id)},
        };
        HttpClient http;
        std::string responce = http.getRequastParam(URL_SERVER_BOT, param);

        nlohmann::json json = nlohmann::json::parse(responce);

        if (json["code"] != "true")
        {
            bot.getApi().sendMessage(message->chat->id, to_utf8(L"Не удалось харегестрироваться."));
            return;
        }

        std::string servUserId = json["id"].dump();    //Добавление зарегестрированного пользователя
        chats.insert(message->chat->id);        //Добавление чата для отправки
        users[std::to_string(message->from->id)] = servUserId;
        saver.saveToFile();
        bot.getApi().sendMessage(message->chat->id, to_utf8(L"Успешная регистрация пользователя!"));
    }
    void renew(TgBot::Message::Ptr message)
    {
        RowJson param =
        {
            {"code_server", "o48c9qw0m4"},
            {"act", API_KEY},
            {"id", getUserIdFromServSafely(message->from->id)}
        };
        HttpClient http;
        std::string responce = http.getRequastParam(URL_SERVER_BOT, param);

        nlohmann::json json = nlohmann::json::parse(responce);
        std::string result;
        if (json["code"].empty())
            result = to_utf8(L"Ошибка обращения к серверу.");
        else if(json["code"] == "true")
            result = to_utf8(L"Сервер продлён.");
        else if(json["code"] == "not reg")
            result = to_utf8(L"Вы не зарегестрированы в тг боте.");
        else if(json["code"] == "already")
            result = to_utf8(L"Сервер уже работает! Проверьте состояние сервера");
        else
            result = to_utf8(L"Сервер не был продлён.");
        bot.getApi().sendMessage(message->chat->id, result);
    }
    void online(TgBot::Message::Ptr message)
    {
        RowJson param = {
            {"code_server", API_KEY},
            {"act", "online"},
            {"id", getUserIdFromServSafely(message->from->id)}
        };
        HttpClient http;
        std::string responce = http.getRequastParam(URL_SERVER_BOT, param);

        nlohmann::json json = nlohmann::json::parse(responce);
        std::string result;
        if (json["code"] != "1")
            result = to_utf8(L"Всего играет: ") + json["online"].dump() + '\n' + json["list"].dump();
        else
            result = to_utf8(L"Ошибка просмотра онлайна.");

        bot.getApi().sendMessage(message->chat->id, result);
    }
    void processMessage(TgBot::Message::Ptr message)
    {
        if (message->text.empty() || message->text[0] == '/')
            return;

        if (!registered(std::to_string(message->from->id)))
            bot.getApi().sendMessage(message->chat->id, to_utf8(L"Сообщение не будет отправлено! Вы не серверный чел!"));
        else
        {
            sendMessageToServer(message->text, std::to_string(message->chat->id));
            std::string mes = '<' + message->from->username + "> " + message->text;
            sendMessageToAllTgExcept(mes/*, message->chat->id*/);
        }
    }

	void initResponsesTG()
	{
        bot.getEvents().onCommand("start", [this](TgBot::Message::Ptr message) {start(message); });
        bot.getEvents().onCommand("renew", [this](TgBot::Message::Ptr message) {renew(message); });
        bot.getEvents().onCommand("online", [this](TgBot::Message::Ptr message) {online(message); });
        bot.getEvents().onAnyMessage([this](TgBot::Message::Ptr message) {processMessage(message); });
	}

    #pragma endregion

    #pragma region clientListener

    void connectSuccess()
    {
        log.log("Connect sucsess");
    }
    void connectFatal()
    {
        log.log("Connect fatal");
    }
    void connectClosed(sio::client::close_reason const& reason)
    {
        log.log("Connect closed. Reaon: ", static_cast<int>(reason));
    }
    void onMessageResponse(sio::event& ev)
    {
        sio::message::ptr data = ev.get_message();

        if (data->get_flag() != sio::message::flag_object)
        {
            log.log("Get message, but its not a object");
            return;
        }
        auto obj = data->get_map();

        auto userIt = obj.find("user");
        auto messageIt = obj.find("text");
        auto typeIt = obj.find("type");

        std::string user = (userIt != obj.end() && userIt->second) ? userIt->second->get_string() : "???";
        std::string message = (messageIt != obj.end() && messageIt->second) ? messageIt->second->get_string() : "";
        std::string type = (typeIt != obj.end() && typeIt->second) ? typeIt->second->get_string() : "";
        if(type != "tg")
            sendMessageToAllTgExcept('<' + user + "> " + message);
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