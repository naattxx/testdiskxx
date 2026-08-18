#include "log.hpp"
#include "spdlog/sinks/basic_file_sink.h"
#include <iostream>

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
        std::cout << "Log init failed: " << ex.what() << std::endl;
        return false;
    }

    return true;
}

void log_close()
{
    file_logger = nullptr;
}
