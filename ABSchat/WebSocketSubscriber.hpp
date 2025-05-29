#pragma once

#include <thread>

#include <jwt-cpp/jwt.h>
#include <sio_client.h>
#include <nlohmann/json.hpp>

#include "ClientBrocker.hpp"
#include "Service.hpp"

class WebSocketSubscriber : public ClientSubscriber
{
public:
    WebSocketSubscriber(ClientBrocker& brocker) :
        ClientSubscriber{}, brocker_{brocker}
	{
        brocker_.addSub(this);
        initListeningSock();
        connectWithServer();
	}

    void sendMessageToYouself(const ClientMessage& message, const TypeMessage type) override
    {
        if (type == TypeMessage::message)
            sendMessageToServer(message);
        else if(type == TypeMessage::voice)
            sendVoiceMessage(message);
    }
	void update() override
	{
        if (!client_.opened())
            reconectTry();
	}

private:

    ClientBrocker& brocker_;

	sio::client client_;

private:

#pragma region Подключение

    void reconectTry()
    {
        Service::log.log("Connectfailed, next try to reconnecd");
        while (!client_.opened())
        {
            connectWithServer();
            std::this_thread::sleep_for(std::chrono::milliseconds(5000));
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
        client_.connect(Service::config.url_to_connect, query);
    }

#pragma endregion

private:

#pragma region Инициализация

	void initListeningSock()
	{
		client_.set_open_listener([this]() {connectSuccess(); });
		client_.set_fail_listener([this]() {connectFatal(); });
		client_.set_close_listener([this](sio::client::close_reason const& reason) {connectClosed(reason); });
		client_.socket()->on("message", [this](sio::event& ev) {onMessageResponse(ev); });
		client_.socket()->on("id_tg_delete", [this](sio::event& ev) {deleteId(ev); });
	}

#pragma endregion

private:

#pragma region Обработка сообщений

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
            return Service::log.log("Get message, but its not a object");
        
        auto obj = data->get_map();

        auto userIt = obj.find("user");
        auto messageIt = obj.find("text");
        auto typeIt = obj.find("type");

        std::string user = (userIt != obj.end() && userIt->second) ? userIt->second->get_string() : "???";
        std::string message = (messageIt != obj.end() && messageIt->second) ? messageIt->second->get_string() : "";
        std::string type = (typeIt != obj.end() && typeIt->second) ? typeIt->second->get_string() : "";

        if (type != "tg")
            brocker_.sendMessage(id_, { '<' + user + "> " + message,0 }, TypeMessage::message);
    }
    void deleteId(sio::event& ev)
    {
        sio::message::ptr data = ev.get_message();

        if (data->get_flag() != sio::message::flag_object)
            return Service::log.log("Get delete_id, but its not a object");
        
        auto obj = data->get_map();
        auto idDelIt = obj.find("id_del");
        std::string idDel = (idDelIt != obj.end() && idDelIt->second) ? idDelIt->second->get_string() : "???";

        brocker_.sendMessage(id_, { "deleteId" , std::stoull(idDel) }, TypeMessage::command);
    }

#pragma endregion

private:

#pragma region Отправка сообщений

    void sendMessageToServer(const ClientMessage& message)
    {
        if (message.first.empty())
            return;

        auto msg = sio::object_message::create();
        msg->get_map()["message"] = sio::string_message::create(message.first);
        msg->get_map()["id"] = sio::string_message::create(std::to_string(message.second));

        client_.socket()->emit("message_tg", msg);
    }
    void sendVoiceMessage(const ClientMessage& message)
    {
        std::string downloadLink = "https://api.telegram.org/file/bot" + Service::config.TG_BOT_KEY + "/" + message.first;

        auto msg = sio::object_message::create();
        msg->get_map()["filePath"] = sio::string_message::create(message.first);
        msg->get_map()["downloadLink"] = sio::string_message::create(downloadLink);
        msg->get_map()["id"] = sio::string_message::create(std::to_string(message.second));

        client_.socket()->emit("voice_message", msg);
    }

#pragma endregion

};