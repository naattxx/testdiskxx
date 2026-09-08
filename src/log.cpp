#include "log.hpp"
#include "spdlog/sinks/basic_file_sink.h"
#include <spdlog/spdlog.h>

using namespace spdlog;

auto log_open(std::string &logfile, TD_LOG create_log) -> bool
{
    if (create_log == TD_LOG::NONE)
        return false;

    try
    {
        file_logger = basic_logger_mt("TestDisk++", logfile, create_log == TD_LOG::CREATE);
    }
    catch (const spdlog_ex &ex)
    {
        spdlog::error("Log init failed: {}", ex.what());
        return false;
    }

    return true;
}

void log_close()
{
    file_logger = nullptr;
}
