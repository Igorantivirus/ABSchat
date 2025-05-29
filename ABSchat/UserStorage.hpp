#pragma once

#include <set>
#include <map>
#include <string>
#include <fstream>

#include <nlohmann/json.hpp>

#include "Service.hpp"

struct UserStorage
{
	UserStorage()
	{
		users[1642467431] = 8;
		chats.insert(1642467431);
	}
	//первое id - с тг, второе с сайта
	std::map<int64_t, int64_t> users;
	std::set<int64_t> chats;

};

class UserFileSaver
{
public:
	UserFileSaver() = default;

	void saveStorageToFile(const UserStorage& storage) const
	{
		nlohmann::json json;
		json["users"] = nlohmann::json::array();  // создаем пустой массив

		for (const auto& [first, second] : storage.users)
			json["users"].push_back({ {"tg_id", first}, {"web_id", second} });
		
		json["chats"] = storage.chats;

		std::ofstream out_file(Service::config.FILE_TO_SAVE);
		if (!out_file.is_open())
			throw std::runtime_error("Failed to open file for writing!");

		out_file << json.dump(4);  // dump(4) — красивое форматирование с отступами
		out_file.close();
	}

	void loadStorageFromFile(UserStorage& storage)
	{
		nlohmann::json json;
		{
			std::ifstream in_file(Service::config.FILE_TO_SAVE);
			if (!in_file.is_open())
				throw std::runtime_error("Failed to open file for reading!");
			in_file >> json;
			in_file.close();
		}

		storage.users.clear();
		storage.chats.clear();

		for (const auto& user_json : json["users"])
		{
			int64_t tg_id = user_json["tg_id"].get<int64_t>();
			int64_t web_id = user_json["web_id"].get<int64_t>();
			storage.users[tg_id] = web_id;
		}

		storage.chats = json["chats"].get<std::set<int64_t>>();
	}

};

class UserStorageAutosaver
{
public:
	UserStorageAutosaver() = default;

	void saveToFile() const
	{
		saver_.saveStorageToFile(storage_);
	}
	void loadFromFile()
	{
		saver_.loadStorageFromFile(storage_);
	}

	void addUser(const int64_t tgId, const int64_t webId)
	{
		storage_.users[tgId] = webId;
		saver_.saveStorageToFile(storage_);
	}
	int64_t getWebIdSafely(const int64_t tgId) const
	{
		auto found = storage_.users.find(tgId);
		if (found == storage_.users.end())
			return 0;
		return found->second;
	}
	void deleteUser(const int64_t tgId)
	{
		storage_.users.erase(tgId);
		saver_.saveStorageToFile(storage_);
	}
	bool isUserRegistered(const int64_t tgId) const
	{
		auto found = storage_.users.find(tgId);
		return found != storage_.users.end();
	}

	void startChat(const int64_t chatId)
	{
		storage_.chats.insert(chatId);
		saver_.saveStorageToFile(storage_);
	}
	void stopChat(const int64_t chatId)
	{
		storage_.chats.erase(chatId);
		saver_.saveStorageToFile(storage_);
	}
	bool isChatOpen(const int64_t chatId) const
	{
		return storage_.chats.find(chatId) != storage_.chats.end();
	}

	const UserStorage& getStorage() const
	{
		return storage_;
	}

private:

	UserStorage storage_;
	UserFileSaver saver_;

};