#pragma once

#include <map>
#include <set>
#include <string>
#include <fstream>

class FileSaver
{
public:
	FileSaver(std::map<std::string, std::string>& usersP, std::set<int64_t>& chatsP, const std::string& name) :
		users{ usersP }, chats{ chatsP }, fileName{name} {}

	void saveToFile() const
	{
		std::ofstream out(fileName);

		out << users.size() << '\n';
		for (const auto& [key, value] : users)
			out << key << ' ' << value << '\n';
		out << chats.size() << '\n';

		for (const auto& i : chats)
			out << i << '\n';
		out.close();
	}

	void readFromFile()
	{
		users.clear();
		chats.clear();

		std::ifstream in(fileName);
		if (!in.is_open())
			return;

		std::size_t prSize;
		std::string prS1, prS2;
		in >> prSize;
		for (std::size_t i = 0; i < prSize; ++i)
		{
			in >> prS1;
			in.get();
			in >> prS2;
			in.get();
			users[prS1] = prS2;
		}
		in >> prSize;
		for (std::size_t i = 0; i < prSize; ++i)
		{
			in >> prS1;
			in.get();

			chats.insert(std::stoll(prS1));
		}
		in.close();
	}

private:

	std::map<std::string, std::string>& users;
	std::set<int64_t>& chats;

	std::string fileName;

};