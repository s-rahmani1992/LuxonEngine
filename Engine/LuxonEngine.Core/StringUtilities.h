#pragma once

#include <string>
#include "export.h"

LUXON_CORE_API std::wstring CharToString(const char* cstr);
LUXON_CORE_API std::string WCharToString(const wchar_t* cstr);
LUXON_CORE_API std::string WStringToString(const std::wstring& wString);