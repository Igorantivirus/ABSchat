#pragma once

#include <locale>
#include <codecvt>
#include <string>

std::string to_utf8(const std::wstring& str) {
	std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
	return converter.to_bytes(str);
}
