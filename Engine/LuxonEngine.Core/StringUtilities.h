#pragma once

#include <string>

std::wstring CharToString(const char* cstr);
std::string WCharToString(const wchar_t* cstr);
std::string WStringToString(const std::wstring& wString);