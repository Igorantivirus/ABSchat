#pragma once

#include <map>
#include <set>
#include <chrono>
#include <thread>
#include <regex>

#include <tgbot/tgbot.h>
#include <sio_client.h>
#include <nlohmann/json.hpp>
#include <jwt-cpp/jwt.h>

#include "HTTPClient.hpp"
#include "HardCode.hpp"
#include "Service.hpp"
#include "UserStorage.hpp"
#include "CommandMeneger.hpp"

using RowJson = std::map<std::string, std::string>;

class Bot
{
public:
    Bot() : bot(Service::config.TG_BOT_KEY)
    {
        storage.loadFromFile();

        initKeyboard();
        initResponsesTG();
        initListeningSock();

        connectWithServer();
    }
    ~Bot()
    {
        storage.saveToFile();
    }

    void run()
    {
        try
        {
            Service::log.log({ "Bot started. Username: ", bot.getApi().getMe()->username.c_str() });
            TgBot::TgLongPoll longPoll(bot);
            while (true)
            {
                if (!client.opened())
                    reconectTry();
                Service::log.log("Long poll started");
                longPoll.start();
            }
        }
        catch (const TgBot::TgException& e)
        {
            Service::log.log({ "Bot error: ",e.what() });
        }
        catch (const std::exception& e)
        {
            Service::log.log({ "Std error: ",e.what() });
        }
        catch (...)
        {
            Service::log.log("Unknown Error");
        }
    }

private:

    TgBot::Bot bot;
    TgBot::ReplyKeyboardMarkup::Ptr keyboard;

    sio::client client;

    UserStorageAutosaver storage;
    CommandMeneger commander;

private:

    void sendMessage(const int64_t chatId, const std::string& message)
    {
#if defined(_WIN32) || defined(_WIN64)
        bot.getApi().sendMessage(chatId, message, false, 0, keyboard);
#else
        bot.getApi().sendMessage(chatId, message, nullptr, 0, keyboard);
#endif // WIN
    }

    void sendMessageToAllTgExcept(const std::string& message, const std::int64_t exceptIdTg = 0)
    {
        for (const auto& chatId : storage.getStorage().chats)
            if(chatId != exceptIdTg)
                sendMessage(chatId, message);
    }

    //проверка, что чат - сообщество и не general
    bool notGeneralInSuperGroup(TgBot::Message::Ptr message)
    {
        return message->chat != nullptr &&
            message->chat->type == TgBot::Chat::Type::Supergroup &&
            message->chat->isForum &&
            message->isTopicMessage;
    }

    #pragma region connectionMethods

    void reconectTry()
    {
        Service::log.log("Connectfailed, next try to reconnecd");
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
        Service::log.log("Connect try");

        std::string token = jwt::create()
            .set_payload_claim("connect_by", jwt::claim(std::string("tg")))
            .set_payload_claim("api_pass", jwt::claim(Service::config.API_KEY))
            .sign(jwt::algorithm::hs256{ Service::config.SUPER_SECRET_KEY });

        std::map<std::string, std::string> query;
        query["token"] = token;
        client.connect(Service::config.url_to_connect, query);
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

    #pragma endregion

    #pragma region botResponser

    void start(TgBot::Message::Ptr message)
    {
        std::string result = commander.startCommand(eraseBeforeForstSpace(message->text), std::to_string(message->from->id));
        if (result.empty())
            return sendMessage(message->chat->id, to_utf8(L"Не удалось зарегестрироваться."));

        storage.startChat(message->chat->id);
        storage.addUser(message->from->id, std::stoll(result));

        sendMessage(message->chat->id, to_utf8(L"Авторизация успешна!"));
    }
    void renew(TgBot::Message::Ptr message)
    {
        if (!storage.isUserRegistered(message->from->id))
            return sendMessage(message->chat->id, to_utf8(L"Вы не участник чата. Команда не будет выполнена."));
        std::string result = commander.renewCommand(std::to_string(storage.getWebIdSafely(message->from->id)));

        if (result == "true")
            return sendMessage(message->chat->id, to_utf8(L"Сервер продлён."));
        if (result == "not reg")
            return sendMessage(message->chat->id, to_utf8(L"Вы не зарегестрированы в тг боте."));
        if (result == "already")
            return sendMessage(message->chat->id, to_utf8(L"Сервер уже работает! Проверьте состояние сервера"));
        sendMessage(message->chat->id, to_utf8(L"Сервер не был продлён."));
    }
    void online(TgBot::Message::Ptr message)
    {
        if (!storage.isUserRegistered(message->from->id))
            return sendMessage(message->chat->id, to_utf8(L"Вы не участник чата. Команда не будет выполнена."));
        std::string result = commander.onlineCommand(std::to_string(storage.getWebIdSafely(message->from->id)));
        if (result.empty())
            return sendMessage(message->chat->id, to_utf8(L"Ошибка просмотра онлайна."));
        sendMessage(message->chat->id, to_utf8(L"Всего играет: ") + result);
    }
    void breakOut(TgBot::Message::Ptr message)
    {
        if (!storage.isUserRegistered(message->from->id))
            return sendMessage(message->chat->id, to_utf8(L"Вы не участник чата. Команда не будет выполнена."));
        std::string result = commander.breakCommand(std::to_string(storage.getWebIdSafely(message->from->id)));
        if (result == "true")
        {
            storage.deleteUser(message->from->id);
            sendMessage(message->chat->id, to_utf8(L"Вы были успешно отвязаны от тг."));
        }
        else
            sendMessage(message->chat->id, to_utf8(L"Отвязка не прошла."));
    }
    void stopChat(TgBot::Message::Ptr message)
    {
        if(!storage.isUserRegistered(message->from->id))
            sendMessage(message->chat->id, to_utf8(L"Вы не участник чата. Команда не будет выполнена."));
        else
        {
            storage.stopChat(message->chat->id);
            sendMessage(message->chat->id, to_utf8(L"Мы больше не будем засорять ваш чат."));
        }
    }
    void startChat(TgBot::Message::Ptr message)
    {
        if (!storage.isUserRegistered(message->from->id))
            sendMessage(message->chat->id, to_utf8(L"Вы не участник сервера - мы не можем работать с вашим чатом."));
        else
        {
            storage.startChat(message->chat->id);
            sendMessage(message->chat->id, to_utf8(L"Теперь мы будем засорять ваш чат."));
        }
    }
    void help(TgBot::Message::Ptr message)
    {
        std::string responce = to_utf8(LR"(Добро пожаловать в бота сервера ABSserver.
Список команд:
/help       - вы сейчас её используете
/online     - проверка онлайна сервера
/renew      - продлить сервер на час
/stopChat   - остановить тправку сообщений в этот чат
/startChat  - возобновить отправку сообщений в этот чат
/keyboard   - вывод клавиатуры, если возникли проблемы с ней
/break      - отвязать аккаунт от телеграмма)");
        sendMessage(message->chat->id, responce);
    }
    void processMessage(TgBot::Message::Ptr message)
    {
        if (!message->text.empty() && message->text[0] == '/')
            return;
        if (!storage.isUserRegistered(message->from->id))
            return sendMessage(message->chat->id, to_utf8(L"Сообщение не будет отправлено! Вы не серверный чел!"));
        if (notGeneralInSuperGroup(message))
            return;
        if (message->voice)
        {
            Service::log.log("VOICE!!!");
            auto file = bot.getApi().getFile(message->voice->fileId);
            std::string downloadLink = "https://api.telegram.org/file/bot" + Service::config.TG_BOT_KEY + "/" + file->filePath;

            auto msg = sio::object_message::create();
            msg->get_map()["filePath"] = sio::string_message::create(file->filePath);
            msg->get_map()["downloadLink"] = sio::string_message::create(downloadLink);
            msg->get_map()["id"] = sio::string_message::create(std::to_string(message->from->id));

            client.socket()->emit("voice_message", msg);
        }
        else if (!storage.isChatOpen(message->chat->id))
            return;
        else if(message->text.size() > 1000)
            sendMessage(message->chat->id, to_utf8(L"Слишком длинное сообщение."));
        else
        {
            sendMessageToServer(message->text, std::to_string(message->from->id));
            std::string mes = '<' + message->from->username + "> " + message->text;
            std::regex_replace(mes, std::regex("\n"), " ");
            sendMessageToAllTgExcept(mes, message->chat->id);
        }
    }

	void initResponsesTG()
	{
        bot.getEvents().onCommand("start", [this](TgBot::Message::Ptr message) { start(message); });
        bot.getEvents().onCommand("renew", [this](TgBot::Message::Ptr message) {renew(message); });
        bot.getEvents().onCommand("online", [this](TgBot::Message::Ptr message) {online(message); });
        bot.getEvents().onCommand("break", [this](TgBot::Message::Ptr message) {breakOut(message); });
        bot.getEvents().onCommand("stopChat", [this](TgBot::Message::Ptr message) {stopChat(message); });
        bot.getEvents().onCommand("startChat", [this](TgBot::Message::Ptr message) {startChat(message); });
        bot.getEvents().onCommand("help", [this](TgBot::Message::Ptr message) {help(message); });
        bot.getEvents().onAnyMessage([this](TgBot::Message::Ptr message) {processMessage(message); });
	}

    #pragma endregion

    #pragma region BotDesigner

    TgBot::KeyboardButton::Ptr makeButton(const std::string& text) const
    {
        TgBot::KeyboardButton::Ptr res(new TgBot::KeyboardButton);
        res->text = text;
        return std::move(res);
    }

    void initKeyboard()
    {
        keyboard = TgBot::ReplyKeyboardMarkup::Ptr(new TgBot::ReplyKeyboardMarkup);
        keyboard->resizeKeyboard = true;
        keyboard->oneTimeKeyboard = false;
        keyboard->keyboard =
        {
            {makeButton("/renew"),makeButton("/online")},
            {makeButton("/stopChat"),makeButton("/startChat")},
            {makeButton("/break"), makeButton("/help")}
        };
    }

    #pragma endregion

    #pragma region clientListener

    void connectSuccess()
    {
        Service::log.log("Connect sucsess");
    }
    void connectFatal()
    {
        Service::log.log("Connect fatal");
    }
    void connectClosed(sio::client::close_reason const& reason)
    {
        Service::log.log("Connect closed. Reaon: ", static_cast<int>(reason));
    }
    void onMessageResponse(sio::event& ev)
    {
        sio::message::ptr data = ev.get_message();

        if (data->get_flag() != sio::message::flag_object)
        {
            Service::log.log("Get message, but its not a object");
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
    void deleteId(sio::event& ev)
    {
        sio::message::ptr data = ev.get_message();

        if (data->get_flag() != sio::message::flag_object)
        {
            Service::log.log("Get delete_id, but its not a object");
            return;
        }
        auto obj = data->get_map();

        auto idDelIt = obj.find("id_del");

        std::string idDel = (idDelIt != obj.end() && idDelIt->second) ? idDelIt->second->get_string() : "???";

        storage.deleteUser(std::stoull(idDel));
    }

    void initListeningSock()
    {
        client.set_open_listener([this]() {connectSuccess(); });
        client.set_fail_listener([this]() {connectFatal(); });
        client.set_close_listener([this](sio::client::close_reason const& reason) {connectClosed(reason); });
        client.socket()->on("message", [this](sio::event& ev) {onMessageResponse(ev); });
        client.socket()->on("id_tg_delete", [this](sio::event& ev) {deleteId(ev); });
    }

    #pragma endregion

};
