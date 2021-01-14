#include "log.h"

#include "spdlog/spdlog.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

#include <string>


namespace ns {

    namespace log {

#ifdef NS_USE_CONSOLE
        static std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> gConsoleLogger;
#endif

        static void initLog(LogLevel aLogLevel) {
#ifdef NS_USE_CONSOLE

            gConsoleLogger = std::make_shared<spdlog::sinks::stdout_color_sink_mt>("console");
            gConsoleLogger->set_pattern(NS_CONSOLE_LOG_PATTERN);

            switch (aLogLevel)
            {
            case ns::log::trace:
                gConsoleLogger->set_level(spdlog::level::trace);
                break;
            case ns::log::debug:
                gConsoleLogger->set_level(spdlog::level::debug);
                break;
            case ns::log::info:
                gConsoleLogger->set_level(spdlog::level::info);
                break;
            case ns::log::warn:
                gConsoleLogger->set_level(spdlog::level::warn);
                break;
            case ns::log::error:
                gConsoleLogger->set_level(spdlog::level::err);
                break;
            case ns::log::critical:
                gConsoleLogger->set_level(spdlog::level::critical);
                break;
            default:
                break;
            }

#endif
            std::string logLocation = NS_LOG_BASE_DIR;
            logLocation.append("app.nslog");

            std::string appName = "app";
#ifdef NS_USE_CONSOLE
            appName = "app_sink";
#endif

            auto appLogger = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                appName,
                logLocation,
                NS_WORKER_LOG_FILE_MAX_SIZE,
                NS_WORKER_LOG_FILE_MAX_NUMBER
            );

            appLogger->set_pattern(NS_APP_LOG_PATTERN);

            switch (aLogLevel)
            {
            case ns::log::trace:
                appLogger->set_level(spdlog::level::trace);
                break;
            case ns::log::debug:
                appLogger->set_level(spdlog::level::debug);
                break;
            case ns::log::info:
                appLogger->set_level(spdlog::level::info);
                break;
            case ns::log::warn:
                appLogger->set_level(spdlog::level::warn);
                break;
            case ns::log::error:
                appLogger->set_level(spdlog::level::err);
                break;
            case ns::log::critical:
                appLogger->set_level(spdlog::level::critical);
                break;
            default:
                break;
            }

#ifdef NS_USE_CONSOLE
            spdlog::logger logger("app", { appLogger, gConsoleLogger });
#endif
            
        }

        static void createWorkerLogger(std::string aLoggerName, LogLevel aLogLevel) {
            std::string logLocation = NS_LOG_BASE_DIR;
            logLocation.append("workers/");
            logLocation.append(aLoggerName);
            logLocation.append("/");
            logLocation.append(aLoggerName);
            logLocation.append(".nslog");

            std::string loggerName = aLoggerName;
#ifdef NS_USE_CONSOLE
            loggerName.append("_sink");
#endif

            auto logger = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                aLoggerName,
                logLocation,
                NS_WORKER_LOG_FILE_MAX_SIZE,
                NS_WORKER_LOG_FILE_MAX_NUMBER
            );

            logger->set_pattern(NS_WORKER_LOG_PATTERN);
            
            switch (aLogLevel)
            {
            case ns::log::trace:
                logger->set_level(spdlog::level::trace);
                break;
            case ns::log::debug:
                logger->set_level(spdlog::level::debug);
                break;
            case ns::log::info:
                logger->set_level(spdlog::level::info);
                break;
            case ns::log::warn:
                logger->set_level(spdlog::level::warn);
                break;
            case ns::log::error:
                logger->set_level(spdlog::level::err);
                break;
            case ns::log::critical:
                logger->set_level(spdlog::level::critical);
                break;
            default:
                break;
            }

#ifdef NS_USE_CONSOLE
            spdlog::logger logger(aLoggerName, { logger, gConsoleLogger });
#endif
        }

    }
}