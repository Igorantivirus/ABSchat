#pragma once

#include <locale>
#include <codecvt>
#include <string>

std::string to_utf8(const std::wstring& str) {
	std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
	return converter.to_bytes(str);
}

std::string eraseBeforeForstSpace(const std::string& str)
{
	std::string res = str;
	if (std::size_t ind = res.find(' '); ind != std::string::npos)
		res.erase(res.begin(), res.begin() + ind + 1);
	return res;
}