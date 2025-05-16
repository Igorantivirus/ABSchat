#include<iostream>

#include <tgbot/tgbot.h>
#include <sio_client.h>
#include <jwt-cpp/jwt.h> 

int main()
{
    system("chcp 65001 > nul");

    std::string secret = "supersecretkey";
    std::string token = jwt::create()
        .set_payload_claim("username", jwt::claim(std::string("fox")))
        .sign(jwt::algorithm::hs256{ secret });



    sio::client client;

    // Подключаем обработчики событий
    client.set_open_listener([&]() {
        std::cout << "Connect sucsess" << std::endl;
        });

    client.set_fail_listener([&]() {
        std::cout << "Error of connect" << std::endl;
        });

    client.set_close_listener([](sio::client::close_reason const& reason) {
        std::cout << "Connect closed. Reaon: " << static_cast<int>(reason) << std::endl;
        });

    client.socket()->on("message", [](sio::event& ev) {
        sio::message::ptr data = ev.get_message();

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
        }
        });

    //std::map<std::string, std::string> query; // можно оставить пустым
    //std::map<std::string, std::string> headers;
    //headers["token"] = "123"; // <-- сюда токен
    //client.connect("http://beta.abserver.ru", query, headers);


    std::map<std::string, std::string> query;
    query["token"] = token;

    std::map<std::string, std::string> headers;


    client.connect("http://beta.abserver.ru", query, headers);

    std::string messageToServer;


    while (true)
    {
        std::string messageToServer;
        std::getline(std::cin, messageToServer);
        if (messageToServer.empty()) continue;

        auto msg = sio::object_message::create();
        msg->get_map()["user"] = sio::string_message::create("fox");   // например, ваш ник
        msg->get_map()["text"] = sio::string_message::create(messageToServer);

        try
        {
            //client.socket()->emit("message", msg);
            client.socket()->emit("message", sio::string_message::create(messageToServer));
        }
        catch (const std::exception& ex)
        {
            std::cerr << "Exception on emit: " << ex.what() << std::endl;
        }
        catch (...)
        {
            std::cerr << "Unknown exception on emit" << std::endl;
        }
    }

    // Завершаем соединение
    client.sync_close();
    client.clear_con_listeners();

    return 0;
}

//int main()
//{
//#if defined(_WIN32) || defined(_WIN64)
//
//
//	system("chcp 65001 > nul");
//
//#endif // __WIN__
//
//
//
//
//	return 0;
//}