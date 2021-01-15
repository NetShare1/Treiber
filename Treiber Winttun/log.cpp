#include "log.h"

#include "spdlog/spdlog.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

#include <string>

namespace ns {

    namespace log {

#ifdef NS_USE_CONSOLE
        std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> gConsoleSink;
#endif


        void initLog(LogLevel aLogLevel) {
            spdlog::level::level_enum logLevel = getSPDLogLevel(aLogLevel);
#ifdef NS_USE_CONSOLE

            gConsoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            gConsoleSink->set_pattern(NS_CONSOLE_LOG_PATTERN);


            gConsoleSink->set_level(logLevel);

#endif
            std::string logLocation = NS_LOG_BASE_DIR;
            logLocation.append("app.nslog");

            auto appSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                logLocation,
                NS_WORKER_LOG_FILE_MAX_SIZE,
                NS_WORKER_LOG_FILE_MAX_NUMBER
            );

            appSink->set_pattern(NS_APP_LOG_PATTERN);

            appSink->set_level(logLevel);

            std::string witnunLogLocation = NS_LOG_BASE_DIR;
            witnunLogLocation.append("wintun/wintun.nslog");

            auto wintunSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                // "wintun",
                witnunLogLocation,
                NS_WORKER_LOG_FILE_MAX_SIZE,
                NS_WORKER_LOG_FILE_MAX_NUMBER
            );

            wintunSink->set_pattern(NS_WINTUN_LOG_PATTERN);

            wintunSink->set_level(logLevel);

            std::vector<spdlog::sink_ptr> appSinks;
            std::vector<spdlog::sink_ptr> wintunSinks;

#ifdef NS_USE_CONSOLE
            appSinks.push_back(appSink);
            appSinks.push_back(gConsoleSink);

            wintunSinks.push_back(wintunSink);
            wintunSinks.push_back(gConsoleSink);

#elif
            appSinks.push_back(appSink);

            wintunSinks.push_back(wintunSink);
#endif

            auto appLogger = std::make_shared<spdlog::logger>(
                "app",
                std::begin(appSinks),
                std::end(appSinks)
            );
            auto wintunLogger = std::make_shared<spdlog::logger>(
                "wintun",
                std::begin(wintunSinks),
                std::end(wintunSinks)
            );

            appLogger->set_level(logLevel);
            wintunLogger->set_level(logLevel);
            spdlog::register_logger(appLogger);
            spdlog::register_logger(wintunLogger);
        }

        void createWorkerLogger(std::string aLoggerName, LogLevel aLogLevel) {
            std::string logLocation = NS_LOG_BASE_DIR;
            logLocation.append("workers/");
            logLocation.append(aLoggerName);
            logLocation.append("/");
            logLocation.append(aLoggerName);
            logLocation.append(".nslog");

            auto fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                logLocation,
                NS_WORKER_LOG_FILE_MAX_SIZE,
                NS_WORKER_LOG_FILE_MAX_NUMBER
            );

            fileSink->set_pattern(NS_WORKER_LOG_PATTERN);
            
            fileSink->set_level(getSPDLogLevel(aLogLevel));

            std::vector<spdlog::sink_ptr> wSinks;

#ifdef NS_USE_CONSOLE
            wSinks.push_back(fileSink);
            wSinks.push_back(gConsoleSink);

#elif
            wSinks.push_back(fileSink);
#endif

            auto wLogger = std::make_shared<spdlog::logger>(
                aLoggerName,
                std::begin(wSinks),
                std::end(wSinks)
            );
            wLogger->set_level(getSPDLogLevel(aLogLevel));
            try {
                spdlog::register_logger(wLogger);
            }
            catch (spdlog::spdlog_ex& err) {
                NS_LOG_APP_WARN("Logger already exists dropping new one");
            }
        }


        void CALLBACK WintunLoggerFunc(_In_ WINTUN_LOGGER_LEVEL Level, _In_z_ const WCHAR* LogLine) {
            std::wstring ws(LogLine);
            std::string str(ws.begin(), ws.end());
            switch (Level)
            {
            case WINTUN_LOG_INFO:
                spdlog::get("wintun")->info(str);
                break;
            case WINTUN_LOG_WARN:
                spdlog::get("wintun")->warn(str);
                break;
            case WINTUN_LOG_ERR:
                spdlog::get("wintun")->error(str);
                break;
            default:
                break;
            }
        }

        spdlog::level::level_enum getSPDLogLevel(LogLevel level) {
            switch (level)
            {
            case ns::log::trace:
                return spdlog::level::trace;
            case ns::log::debug:
                return spdlog::level::debug;
            case ns::log::info:
                return spdlog::level::info;
            case ns::log::warn:
                return spdlog::level::warn;
            case ns::log::error:
                return spdlog::level::err;
            case ns::log::critical:
                return spdlog::level::critical;
            default:
                return spdlog::level::info;
            }
        }

    }
}