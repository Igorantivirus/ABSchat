#define _CRT_SECURE_NO_WARNINGS

#include "Bot.hpp"

#include "UserStorage.hpp"

int main()
{
#if defined(_WIN32) || defined(_WIN64)
	system("chcp 65001 > nul");
#endif // __WIN__

	Service::init("config.json");

	/*UserStorageAutosaver users;

	users.saveToFile();
	users.loadFromFile();*/

	/*UserStorage stor;
	UserFileSaver saver;

	saver.saveStorageToFile(stor);
	saver.loadStorageFromFile(stor);*/



    Bot bot;

	bot.run();

	return 0;
}
