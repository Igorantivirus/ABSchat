#pragma once

#include <map>
#include <set>
#include <string>

class FileSaver
{
public:
	FileSaver(std::map<std::string, std::string>& usersP, std::set<int64_t>& chatsP) :
		users{ usersP }, chats{ chatsP } {}

private:

	std::map<std::string, std::string>& users;
	std::set<int64_t>& chats;



};