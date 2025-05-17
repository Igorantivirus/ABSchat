#include<iostream>

#include <curl/curl.h>
#include <tgbot/tgbot.h>
#include <sio_client.h>
#include <jwt-cpp/jwt.h> 

#include "KEYS.hpp"
#include "Bot.hpp"

int main()
{
#if defined(_WIN32) || defined(_WIN64)
	system("chcp 65001 > nul");
#endif // __WIN__

	Bot bot(TG_BOT_KEY);

	bot.run();

	return 0;
}