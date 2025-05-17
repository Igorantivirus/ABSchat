#pragma once

#include <string>
#include <fstream>
#include <chrono>
#include <initializer_list>

#include "HardCode.hpp"

class LogOutput
{
public:
	LogOutput(const std::string& fileName, const bool writeCoConsole = false) :
		toConsole(writeCoConsole)
	{
		logout.open(fileName, std::ios_base::app);
	}
	~LogOutput()
	{
		logout.close();
	}

	void setConsoleMode(const bool writeCoConsole)
	{
		toConsole = writeCoConsole;
	}
	bool isComsoleWriting() const
	{
		return toConsole;
	}

	void log(const std::string& message, const int code = 0)
	{
		std::string logMessage = '[' + curentDate() + ']' +
			" Message: \"" + message +
			"\". Additionsl code = " + std::to_string(code) + ".\n";

		logout << logMessage;
		if (toConsole)
			std::cout << logMessage;
	}
	void log(const std::wstring& message, const int code = 0)
	{
		log(to_utf8(message), code);
	}

	void log(const std::initializer_list<std::string> list, const int code = 0)
	{
		std::string message;
		
		for (const auto& l : list)
			message += l;
		log(message, code);
	}
	void log(const std::initializer_list<std::wstring> list, const int code = 0)
	{
		std::wstring message;

		for (const auto& l : list)
			message += l;
		log(message, code);
	}

private:

	std::ofstream logout;

	bool toConsole = false;

	static std::string curentDate()
	{
		std::time_t end_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		std::string res = ctime(&end_time);
		res.pop_back();
		return res;
	}

};