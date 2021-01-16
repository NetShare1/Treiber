#pragma once

#include "wintun.h"

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"

#define NS_WORKER_LOG_PATTERN "[%H:%M:%S.%f] [%L] [thread %t] %v"
#define NS_CONSOLE_LOG_PATTERN "[%H:%M:%S.%f] [%n] [%^%=8l%$] [thread %t] %v"
#define NS_APP_LOG_PATTERN "[%H:%M:%S.%f] [%L] [thread %t] %v"
#define NS_WINTUN_LOG_PATTERN "[%H:%M:%S.%f] [%L] [thread %t] %v"

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
        extern std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> gConsoleSink;
#endif


        void initLog(LogLevel aLogLevel);
        void createWorkerLogger(std::string aLoggerName, LogLevel aLogLevel);
        void CALLBACK WintunLoggerFunc(_In_ WINTUN_LOGGER_LEVEL Level, _In_z_ const WCHAR* LogLine);
        spdlog::level::level_enum getSPDLogLevel(LogLevel level);
    }
}

#define NS_INIT_LOG ns::log::initLog
#define NS_CREATE_WORKER_LOGGER ns::log::createWorkerLogger
