#pragma once

#include <map>
#include <set>
#include <chrono>
#include <thread>
#include <regex>

#include <tgbot/tgbot.h>
#include <nlohmann/json.hpp>
#include <sio_client.h>
#include <jwt-cpp/jwt.h>
#include <boost/asio.hpp>

#include "HTTPClient.hpp"
#include "HardCode.hpp"
#include "FileSaver.hpp"
#include "LogOutput.hpp"
#include "Config.hpp"

using RowJson = std::map<std::string, std::string>;

class Bot
{
    Config config;

    boost::asio::io_context io_context_;
    boost::asio::ip::tcp::acceptor acceptor_;
    std::mutex file_mutex_;
    bool running_ = true;
    std::thread server_thread_;
public:
    Bot(const std::string configPath) :
                               config(loadConfig(configPath)), bot(config.TG_BOT_KEY), saver{ users, chats, config.FILE_TO_SAVE }, log{ config.LOG_FILE, true}, acceptor_(io_context_, boost::asio::ip::tcp::endpoint(boost::asio::ip::make_address(config.minecraft_server_ip), config.minecraft_server_port))
    {
        saver.readFromFile();

        initResponsesTG();
        initListeningSock();

        connectWithServer();

        server_thread_ = std::thread([this]() { this->mc_listener_run(); });
    }
    ~Bot()
    {
        saver.saveToFile();

        // Signal thread to stop and wait for it
        running_ = false;
        io_context_.stop();
        if (server_thread_.joinable()) { server_thread_.join(); }
    }

    void mc_listener_run() {
        while (running_) {
            try {
                boost::asio::ip::tcp::socket socket(io_context_);
                acceptor_.accept(socket);

                handleClient(std::move(socket));
            } catch (const std::exception &e) {
                if (running_) {
                    std::cerr << "Server error: " << e.what() << std::endl;
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }
            }
        }
    }

    void handleClient(boost::asio::ip::tcp::socket socket) {
        try {
            uint32_t message_size = 0;
            boost::asio::read(socket, boost::asio::buffer(&message_size, sizeof(message_size)));
            std::vector<char> data(message_size);
            boost::asio::read(socket, boost::asio::buffer(data));
            std::string message(data.begin(), data.end());
            onMinecraftChatMessage(message);
        } catch (const std::exception &e) { std::cerr << "Error handling client: " << e.what() << std::endl; }
    }

    void parseMessage(const std::string& input, std::string& username, std::string& message) {
        // Find the positions of < and >
        size_t openBracket = input.find('<');
        size_t closeBracket = input.find('>');

        // Extract username (without brackets)
        if (openBracket != std::string::npos && closeBracket != std::string::npos && openBracket < closeBracket) {
            username = input.substr(openBracket + 1, closeBracket - openBracket - 1);
        } else {
            username = ""; // In case of invalid format
        }

        // Extract message (skipping the colon and any spaces after it)
        if (closeBracket != std::string::npos) {
            size_t messageStart = closeBracket + 1;
            // Skip any spaces after the colon
            while (messageStart < input.length() && input[messageStart] == ' ') {
                messageStart++;
            }
            message = input.substr(messageStart);
        } else {
            message = ""; // In case of invalid format
        }
    }

    void onMinecraftChatMessage(const std::string& message) {
        if (message.empty())
            return;
        std::cout << "Message sent!!!" << std::endl;

        auto msg = sio::object_message::create();
        std::string username, only_message;
        parseMessage(message, username, only_message);
        msg->get_map()["user"] = sio::string_message::create(username);
        msg->get_map()["message"] = sio::string_message::create(only_message);
        msg->get_map()["type"] = sio::string_message::create("mine");
        //msg->get_map()["id"] = sio::string_message::create("1642467431");

        client.socket()->emit("message", msg);
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
            .sign(jwt::algorithm::hs256{ config.SUPER_SECRET_KEY });

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
        std::string responce = http.getRequastParam(config.URL_SERVER_BOT, param);

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
            {"act", config.API_KEY},
            {"id", getUserIdFromServSafely(message->from->id)}
        };
        HttpClient http;
        std::string responce = http.getRequastParam(config.URL_SERVER_BOT, param);

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
            {"code_server", config.API_KEY},
            {"act", "online"},
            {"id", getUserIdFromServSafely(message->from->id)}
        };
        HttpClient http;
        std::string responce = http.getRequastParam(config.URL_SERVER_BOT, param);

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
            std::regex_replace(mes, std::regex("\n"), " ");
            sendMessageToAllTgExcept(mes, message->chat->id);
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
