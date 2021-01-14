#ifndef LOG
#define LOG

#include "wintun.h"
#include "init_wintun.h"
#include <stdio.h>
#include "minitrace.h"

#if 0

static void CALLBACK
    ConsoleLogger(_In_ WINTUN_LOGGER_LEVEL Level, _In_z_ const WCHAR* LogLine)
{
    MTR_SCOPE("Logging", __FUNCSIG__);
    FILETIME Timestamp;
    GetSystemTimePreciseAsFileTime(&Timestamp);
    SYSTEMTIME SystemTime;
    FileTimeToSystemTime(&Timestamp, &SystemTime);
    WCHAR LevelMarker;
    switch (Level)
    {
    case WINTUN_LOG_INFO:
        LevelMarker = L'+';
        break;
    case WINTUN_LOG_WARN:
        LevelMarker = L'-';
        break;
    case WINTUN_LOG_ERR:
        LevelMarker = L'!';
        break;
    default:
        return;
    }
    fwprintf(
        stderr,
        L"%04d-%02d-%02d %02d:%02d:%02d.%04d [%c] %s\n",
        SystemTime.wYear,
        SystemTime.wMonth,
        SystemTime.wDay,
        SystemTime.wHour,
        SystemTime.wMinute,
        SystemTime.wSecond,
        SystemTime.wMilliseconds,
        LevelMarker,
        LogLine);
}

static DWORD
    LogError(_In_z_ const WCHAR* Prefix, _In_ DWORD Error)
{
    MTR_SCOPE("Logging", __FUNCSIG__);
    WCHAR* SystemMessage = NULL, * FormattedMessage = NULL;
    FormatMessageW(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_MAX_WIDTH_MASK,
        NULL,
        HRESULT_FROM_SETUPAPI(Error),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        SystemMessage,
        0,
        NULL);

    DWORD_PTR dwptrarr[] = { (DWORD_PTR)Prefix, (DWORD_PTR)Error, (DWORD_PTR)SystemMessage };

    FormatMessageW(
        FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_ARGUMENT_ARRAY |
        FORMAT_MESSAGE_MAX_WIDTH_MASK,
        SystemMessage ? L"%1: %3(Code 0x%2!08X!)" : L"%1: Code 0x%2!08X!",
        0,
        0,
        FormattedMessage,
        0,
        (va_list*)dwptrarr
    );
    if (FormattedMessage)
        ConsoleLogger(WINTUN_LOG_ERR, FormattedMessage);
    LocalFree(FormattedMessage);
    LocalFree(SystemMessage);
    LocalFree(dwptrarr); // idk if localfree is the right one
    return Error;
}

static DWORD
    LogLastError(_In_z_ const WCHAR* Prefix)
{
    MTR_SCOPE("Logging", __FUNCSIG__);
    DWORD LastError = GetLastError();
    LogError(Prefix, LastError);
    SetLastError(LastError);
    return LastError;
}

static void
    Log(_In_ WINTUN_LOGGER_LEVEL Level, _In_z_ const WCHAR* Format, ...)
{
    MTR_SCOPE("Logging", __FUNCSIG__);
    WCHAR LogLine[0x200];
    va_list args;
    va_start(args, Format);
    _vsnwprintf_s(LogLine, _countof(LogLine), _TRUNCATE, Format, args);
    va_end(args);
    ConsoleLogger(Level, LogLine);
}
#endif

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"

#define NS_USE_CONSOLE

#define NS_WORKER_LOG_PATTERN ""
#define NS_CONSOLE_LOG_PATTERN ""
#define NS_APP_LOG_PATTERN ""

#define NS_WORKER_LOG_FILE_MAX_SIZE 1048576 * 5
#define NS_WORKER_LOG_FILE_MAX_NUMBER 3
#define NS_LOG_BASE_DIR "logs/"

#define NS_LOG_TRACE(aLoggerName, ...) spdlog::get(aLoggerName)->trace(##__VA_ARGS__)
#define NS_LOG_DEBUG(aLoggerName, ...) spdlog::get(aLoggerName)->debug(##__VA_ARGS__)
#define NS_LOG_INFO(aLoggerName, ...) spdlog::get(aLoggerName)->info(##__VA_ARGS__)
#define NS_LOG_WARN(aLoggerName, ...) spdlog::get(aLoggerName)->warn(##__VA_ARGS__)
#define NS_LOG_ERROR(aLoggerName, ...) spdlog::get(aLoggerName)->error(##__VA_ARGS__)
#define NS_LOG_CRITICAL(aLoggerName, ...) spdlog::get(aLoggerName)->critical(##__VA_ARGS__)

#define NS_LOG_APP_TRACE spdlog::get("app")->trace
#define NS_LOG_APP_DEBUG spdlog::get("app")->debug
#define NS_LOG_APP_INFO spdlog::get("app")->info
#define NS_LOG_APP_WARN spdlog::get("app")->warn
#define NS_LOG_APP_ERROR spdlog::get("app")->error
#define NS_LOG_APP_CRITICAL spdlog::get("app")->critical

namespace ns {

    namespace log {

        enum LogLevel {
            trace    = 0,
            debug    = 1,
            info     = 2,
            warn     = 3,
            error    = 4,
            critical = 5
        };

#ifdef NS_USE_CONSOLE
        static std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> gConsoleLogger;
#endif


        static void initLog(LogLevel aLogLevel);
        static void createWorkerLogger(std::string aLoggerName, LogLevel aLogLevel);

    }
}

#define NS_INIT_LOG ns::log::initLog
#define NS_CREATE_WORKER_LOGGER ns::log:createWorkerLogger

#endif // LOG