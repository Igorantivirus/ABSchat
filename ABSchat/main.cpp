#define _CRT_SECURE_NO_WARNINGS

#include<iostream>

#include <curl/curl.h>
#include <tgbot/tgbot.h>
#include <sio_client.h>
#include <jwt-cpp/jwt.h>

#include "Bot.hpp"

int main()
{
#if defined(_WIN32) || defined(_WIN64)
	system("chcp 65001 > nul");
#endif // __WIN__

    Bot bot("config.json");

	bot.run();

	return 0;
}
