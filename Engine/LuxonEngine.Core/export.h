#pragma once

#if defined(LUXON_CORE_EXPORT)
#  define LUXON_CORE_API __declspec(dllexport)
#else
#  define LUXON_CORE_API __declspec(dllimport)
#endif
