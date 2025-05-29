#define _CRT_SECURE_NO_WARNINGS

#include "Bot.hpp"

int main()
{
#if defined(_WIN32) || defined(_WIN64)
	system("chcp 65001 > nul");
#endif // __WIN__

	Service::init("config.json");

    Bot bot;

	bot.run();

	return 0;
}
