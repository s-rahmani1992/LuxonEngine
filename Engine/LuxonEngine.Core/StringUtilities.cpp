#include "StringUtilities.h"
#include "Platform/CommonWin.h" //TODO Fix <stringapi.h> and <Windows.h> conflict

std::wstring CharToString(const char* cstr) {
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, cstr, (int)strlen(cstr), nullptr, 0);
	std::wstring wcstr(size_needed, 0);
	MultiByteToWideChar(CP_UTF8, 0, cstr, (int)strlen(cstr), wcstr.data(), size_needed);
	return wcstr;
}

std::string WCharToString(const wchar_t* wcstr)
{
	int size_needed = WideCharToMultiByte(CP_UTF8, 0, wcstr, -1, nullptr, 0, nullptr, nullptr);
	std::string str(size_needed - 1, 0); // exclude null terminator
	WideCharToMultiByte(CP_UTF8, 0, wcstr, -1, str.data(), size_needed, nullptr, nullptr);
	return str;
}

std::string WStringToString(const std::wstring& wString)
{
	return WCharToString(wString.c_str());
}
