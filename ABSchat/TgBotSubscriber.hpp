#pragma once

#include <regex>

#include <tgbot/tgbot.h>

#include "ClientBrocker.hpp"
#include "ChatsList.hpp"

class TgBotSubscriber : public ClientSubscriber
{
public:
    TgBotSubscriber(ClientBrocker& brocker) :
        ClientSubscriber{}, brocker_{ brocker }, bot_{ Service::config.TG_BOT_KEY }, longPoll_{bot_}
    {
        brocker_.addSub(this);

        chats_ = ChatsList::getActualChats();

        initKeyboard();
        initResponses();
        
        Service::log.log({ "Bot started. Username: ", bot_.getApi().getMe()->username.c_str() });
    }

    void sendMessageToYouself(const ClientMessage& message, const TypeMessage type) override
    {
        if(type == TypeMessage::message)
            sendMessageToAllTgExcept(message);
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

    std::set<int64_t> chats_;

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
            {makeButton("/help"),makeButton("/online")},
            {makeButton("/updateChats"), makeButton("/start")}
        };
    }

    void initResponses()
    {
        bot_.getEvents().onCommand("start",         [this](TgBot::Message::Ptr message) {start(message); });
        bot_.getEvents().onCommand("online",        [this](TgBot::Message::Ptr message) {online(message); });
        bot_.getEvents().onCommand("help",          [this](TgBot::Message::Ptr message) {help(message); });
        bot_.getEvents().onCommand("updateChats",   [this](TgBot::Message::Ptr message) {updateChats(message); });
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
        bot_.getApi().sendMessage(chatId, message, nullptr, 0, keyboard_);
#endif // WIN
    }

    void sendMessageToAllTgExcept(const std::string& message, const std::int64_t exceptIdTg = 0) const
    {
        for (const auto& chatId : chats_)
            if (chatId != exceptIdTg)
                sendMessage(chatId, message);
    }

#pragma endregion

private:

#pragma region Обработка сообщений

    bool isAdmin(TgBot::Message::Ptr message) const
    {
        if (message->chat->type != TgBot::Chat::Type::Supergroup && message->chat->type != TgBot::Chat::Type::Group)
            return false;
        auto admins = bot_.getApi().getChatAdministrators(message->chat->id);
        for (const auto& admin : admins)
            if (admin->user->id == message->from->id)
                return true;
        return false;
    }
    bool isChatIsOpen(const int64_t id) const
    {
        return chats_.count(id);
    }

    void start(TgBot::Message::Ptr message) const
    {
        std::string result = to_utf8(L"Добро пожаловать в бот сервера ABServer. ID вашего чата: `") + std::to_string(message->chat->id) + '`';
        sendMessage(message->chat->id, result);
    }
    void online(TgBot::Message::Ptr message) const
    {
        sendMessage(message->chat->id, to_utf8(L"К сожалению, сейчас эта команда не доступна. Ждите обновления."));
    }
    void help(TgBot::Message::Ptr message) const
    {
        std::string responce = to_utf8(LR"(Добро пожаловать в бота сервера ABSserver.
Список команд:
/help        - вы сейчас её используете
/online      - проверка онлайна сервера
/updateChats - проверь, можно ли кидать сообщения в ваш чат)");
        sendMessage(message->chat->id, responce);
    }
    void updateChats(TgBot::Message::Ptr message)
    {
        if (message->chat->type != TgBot::Chat::Type::Private && !isAdmin(message))
            return;
        chats_ = ChatsList::getActualChats();
        std::string result;
        if (isChatIsOpen(message->chat->id))
            result = to_utf8(L"Ваш чат активен!");
        else
            result = to_utf8(L"Ваш чат не активен!");
        sendMessage(message->chat->id, result);
    }
    void processMessage(TgBot::Message::Ptr message)
    {
        if
        (
            (!message->text.empty() && message->text[0] == '/') ||
            (message->text.empty() && !message->voice) ||
            !isChatIsOpen(message->chat->id) ||
            notGeneralInSuperGroup(message)
        )
            return;
        if (message->text.size() > 1000)
            return sendMessage(message->chat->id, to_utf8(L"Слишком длинное сообщение."));

        if (message->voice)
        {
            Service::log.log("VOICE!!!");
            TgBot::File::Ptr file = bot_.getApi().getFile(message->voice->fileId);
            brocker_.sendMessage(id_, file->filePath, TypeMessage::voice);
        }
        else
        {
            std::string result = '<' + message->from->username + '>' + ' ' + message->text;
            brocker_.sendMessage(id_, result, TypeMessage::message);
            sendMessageToAllTgExcept(result, message->chat->id);
        }
    }

#pragma endregion

};
