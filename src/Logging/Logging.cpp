#include "Logging.h"

void Logging::Initialize()
{
    auto pConsole = spdlog::stdout_color_mt("console");
    spdlog::set_default_logger(pConsole);

    spdlog::set_pattern("%^[%T] [%-5l]%$ %v");
    spdlog::set_level(spdlog::level::info);

    SPDLOG_INFO("Logging is initialized.");
}