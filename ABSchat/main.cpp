#define _CRT_SECURE_NO_WARNINGS

#include "TgBotSubscriber.hpp"
#include "MinecraftChatSubscriber.hpp"

int main()
{
#if defined(_WIN32) || defined(_WIN64)
	system("chcp 65001 > nul");
#endif // __WIN__

	ClientBrocker brocker;

	TgBotSubscriber tg(brocker);
	MinecraftChatSubscriber mine(brocker);

	brocker.start();

	return 0;
}
