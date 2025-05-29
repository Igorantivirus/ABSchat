#pragma once

#include "Config.hpp"
#include "LogOutput.hpp"

class Service
{
public:

	static void init(const std::string& configFile)
	{
		config = loadConfig(configFile);
		log.init(config.LOG_FILE, true);
	}

	static Config config;
	static LogOutput log;

};

Config Service::config{};
LogOutput Service::log{};