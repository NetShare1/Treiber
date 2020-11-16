#ifndef LOG
#define LOG

#include "wintun.h"
#include "init_wintun.h"


static void CALLBACK
ConsoleLogger(_In_ WINTUN_LOGGER_LEVEL Level, _In_z_ const WCHAR* LogLine);

static DWORD
LogError(_In_z_ const WCHAR* Prefix, _In_ DWORD Error);

static DWORD
LogLastError(_In_z_ const WCHAR* Prefix);

static void
Log(_In_ WINTUN_LOGGER_LEVEL Level, _In_z_ const WCHAR* Format, ...);

#endif // LOG