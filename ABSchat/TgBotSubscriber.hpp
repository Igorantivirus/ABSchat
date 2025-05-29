#pragma once

#include <regex>

#include <tgbot/tgbot.h>
#include <nlohmann/json.hpp>

#include "ClientBrocker.hpp"
#include "CommandMeneger.hpp"
#include "UserStorage.hpp"

class TgBotSubscriber : public ClientSubscriber
{
public:
    TgBotSubscriber(ClientBrocker& brocker) :
        ClientSubscriber{}, brocker_{ brocker }, bot_{ Service::config.TG_BOT_KEY }, longPoll_{bot_}
    {
        brocker_.addSub(this);
        storage_.loadFromFile();
        initKeyboard();
        initResponses();
        Service::log.log({ "Bot started. Username: ", bot_.getApi().getMe()->username.c_str() });
    }
    ~TgBotSubscriber()
    {
        storage_.saveToFile();
    }

    void sendMessageToYouself(const ClientMessage& message, const TypeMessage type) override
    {
        if(type == TypeMessage::message)
            sendMessageToAllTgExcept(message.first);
        else if (type == TypeMessage::command && message.first == "deleteId")
            storage_.deleteUser(message.second);
    }
    void update() override
    {
        Service::log.log("Long poll started");
        longPoll_.start();
    }

private:

    ClientBrocker& brocker_;

    TgBot::Bot bot_;
    TgBot::ReplyKeyboardMarkup::Ptr keyboard_;
    TgBot::TgLongPoll longPoll_;

    UserStorageAutosaver storage_;
    CommandMeneger commander_;

private:

#pragma region Статичные методы

    //проверка, что чат - сообщество и не general
    static bool notGeneralInSuperGroup(TgBot::Message::Ptr message)
    {
        return message->chat != nullptr &&
            message->chat->type == TgBot::Chat::Type::Supergroup &&
            message->chat->isForum &&
            message->isTopicMessage;
    }

    static TgBot::KeyboardButton::Ptr makeButton(const std::string& text)
    {
        TgBot::KeyboardButton::Ptr res(new TgBot::KeyboardButton);
        res->text = text;
        return std::move(res);
    }

#pragma endregion

private:

#pragma region Инициализация

    void initKeyboard()
    {
        keyboard_ = TgBot::ReplyKeyboardMarkup::Ptr(new TgBot::ReplyKeyboardMarkup);
        keyboard_->resizeKeyboard = true;
        keyboard_->oneTimeKeyboard = false;
        keyboard_->keyboard =
        {
            {makeButton("/renew"),makeButton("/online")},
            {makeButton("/stopChat"),makeButton("/startChat")},
            {makeButton("/break"), makeButton("/help")}
        };
    }

    void initResponses()
    {
        bot_.getEvents().onCommand("start", [this](TgBot::Message::Ptr message) { start(message); });
        bot_.getEvents().onCommand("renew", [this](TgBot::Message::Ptr message) {renew(message); });
        bot_.getEvents().onCommand("online", [this](TgBot::Message::Ptr message) {online(message); });
        bot_.getEvents().onCommand("break", [this](TgBot::Message::Ptr message) {breakOut(message); });
        bot_.getEvents().onCommand("stopChat", [this](TgBot::Message::Ptr message) {stopChat(message); });
        bot_.getEvents().onCommand("startChat", [this](TgBot::Message::Ptr message) {startChat(message); });
        bot_.getEvents().onCommand("help", [this](TgBot::Message::Ptr message) {help(message); });
        bot_.getEvents().onAnyMessage([this](TgBot::Message::Ptr message) {processMessage(message); });
    }

#pragma endregion

private:

#pragma region Отправка сообщений в чаты

    void sendMessage(const int64_t chatId, const std::string& message) const
    {
#if defined(_WIN32) || defined(_WIN64)
        bot_.getApi().sendMessage(chatId, message, false, 0, keyboard_);
#else
        bot_.getApi().sendMessage(chatId, message, nullptr, 0, keyboard);
#endif // WIN
    }

    void sendMessageToAllTgExcept(const std::string& message, const std::int64_t exceptIdTg = 0) const
    {
        for (const auto& chatId : storage_.getStorage().chats)
            if (chatId != exceptIdTg)
                sendMessage(chatId, message);
    }

#pragma endregion

private:

#pragma region Обработка сообщений

    void start(TgBot::Message::Ptr message)
    {
        std::string result = commander_.startCommand(eraseBeforeForstSpace(message->text), std::to_string(message->from->id));
        if (result.empty())
            return sendMessage(message->chat->id, to_utf8(L"Не удалось зарегестрироваться."));

        storage_.startChat(message->chat->id);
        storage_.addUser(message->from->id, std::stoll(result));

        sendMessage(message->chat->id, to_utf8(L"Авторизация успешна!"));
    }
    void renew(TgBot::Message::Ptr message) const
    {
        if (!storage_.isUserRegistered(message->from->id))
            return sendMessage(message->chat->id, to_utf8(L"Вы не участник чата. Команда не будет выполнена."));
        std::string result = commander_.renewCommand(std::to_string(storage_.getWebIdSafely(message->from->id)));

        if (result == "true")
            return sendMessage(message->chat->id, to_utf8(L"Сервер продлён."));
        if (result == "not reg")
            return sendMessage(message->chat->id, to_utf8(L"Вы не зарегестрированы в тг боте."));
        if (result == "already")
            return sendMessage(message->chat->id, to_utf8(L"Сервер уже работает! Проверьте состояние сервера"));
        sendMessage(message->chat->id, to_utf8(L"Сервер не был продлён."));
    }
    void online(TgBot::Message::Ptr message) const
    {
        if (!storage_.isUserRegistered(message->from->id))
            return sendMessage(message->chat->id, to_utf8(L"Вы не участник чата. Команда не будет выполнена."));
        std::string result = commander_.onlineCommand(std::to_string(storage_.getWebIdSafely(message->from->id)));
        if (result.empty())
            return sendMessage(message->chat->id, to_utf8(L"Ошибка просмотра онлайна."));
        sendMessage(message->chat->id, to_utf8(L"Всего играет: ") + result);
    }
    void breakOut(TgBot::Message::Ptr message)
    {
        if (!storage_.isUserRegistered(message->from->id))
            return sendMessage(message->chat->id, to_utf8(L"Вы не участник чата. Команда не будет выполнена."));
        std::string result = commander_.breakCommand(std::to_string(storage_.getWebIdSafely(message->from->id)));
        if (result == "true")
        {
            storage_.deleteUser(message->from->id);
            sendMessage(message->chat->id, to_utf8(L"Вы были успешно отвязаны от тг."));
        }
        else
            sendMessage(message->chat->id, to_utf8(L"Отвязка не прошла."));
    }
    void stopChat(TgBot::Message::Ptr message)
    {
        if (!storage_.isUserRegistered(message->from->id))
            sendMessage(message->chat->id, to_utf8(L"Вы не участник чата. Команда не будет выполнена."));
        else
        {
            storage_.stopChat(message->chat->id);
            sendMessage(message->chat->id, to_utf8(L"Мы больше не будем засорять ваш чат."));
        }
    }
    void startChat(TgBot::Message::Ptr message)
    {
        if (!storage_.isUserRegistered(message->from->id))
            sendMessage(message->chat->id, to_utf8(L"Вы не участник сервера - мы не можем работать с вашим чатом."));
        else
        {
            storage_.startChat(message->chat->id);
            sendMessage(message->chat->id, to_utf8(L"Теперь мы будем засорять ваш чат."));
        }
    }
    void help(TgBot::Message::Ptr message) const
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
        if
        (
            (!message->text.empty() && message->text[0] == '/') ||
            (message->text.empty() && !message->voice) ||
            !storage_.isChatOpen(message->chat->id) ||
            notGeneralInSuperGroup(message)
        )
            return;
        if (!storage_.isUserRegistered(message->from->id))
            return sendMessage(message->chat->id, to_utf8(L"Сообщение не будет отправлено! Вы не зарегистрированы!"));
        if (message->text.size() > 1000)
            sendMessage(message->chat->id, to_utf8(L"Слишком длинное сообщение."));

        if (message->voice)
        {
            Service::log.log("VOICE!!!");
            TgBot::File::Ptr file = bot_.getApi().getFile(message->voice->fileId);
            brocker_.sendMessage(id_, { file->filePath, message->from->id }, TypeMessage::voice);
        }
        else
        {
            brocker_.sendMessage(id_, { message->text , message->from->id }, TypeMessage::message);

            std::string mes = '<' + message->from->username + "> " + message->text;
            std::regex_replace(mes, std::regex("\n"), " ");
            sendMessageToAllTgExcept(mes, message->chat->id);
        }
    }

#pragma endregion

};
