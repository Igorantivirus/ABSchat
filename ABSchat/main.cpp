#define _CRT_SECURE_NO_WARNINGS

#include "Bot.hpp"
//6928361979:AAG4-B4eEcfwDcxcuYY6mgMqpTRjtT2f1Q8
int main()
{
#if defined(_WIN32) || defined(_WIN64)
	system("chcp 65001 > nul");
#endif // __WIN__

    Bot bot("config.json");

	bot.run();

	return 0;
}
