#pragma once

#include <memory>
#include <spdlog/spdlog.h>
#include <string>

using namespace spdlog;

inline std::shared_ptr<logger> file_logger = nullptr;

enum class TD_LOG
{
    NONE,
    CREATE,
    APPEND,
    DONE
};

#define log_info(...) (file_logger ? file_logger->info(__VA_ARGS__) : (void)0)
#define log_error(...) (file_logger ? file_logger->error(__VA_ARGS__) : (void)0)
#define log_warning(...) (file_logger ? file_logger->warn(__VA_ARGS__) : (void)0)
#define log_critical(...) (file_logger ? file_logger->critical(__VA_ARGS__) : (void)0)
#define log_verbose(...) (file_logger ? file_logger->verbose(__VA_ARGS__) : (void)0)
#define log_debug(...) (file_logger ? file_logger->debug(__VA_ARGS__) : (void)0)
#define log_trace(...) (file_logger ? file_logger->trace(__VA_ARGS__) : (void)0)

bool log_open(std::string &logfile, TD_LOG create_log);
void log_close();
