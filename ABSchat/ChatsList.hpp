#pragma once

#include <fstream>
#include <string>
#include <set>

#include <nlohmann/json.hpp>

#include "Service.hpp"

class ChatsList
{
public:

	static std::set<int64_t> getActualChats()
	{
		nlohmann::json json;

		{
			std::ifstream in_file(Service::config.CHATS_FILE);
			if (!in_file.is_open())
				throw std::runtime_error("Failed to open file for reading!");
			in_file >> json;
			in_file.close();
		}

		return json["chats"].get<std::set<int64_t>>();
	}

private:



};