#include "Logging.h"

void Logging::Initialize()
{
    auto console = spdlog::stdout_color_mt("console");
    spdlog::set_default_logger(console);

    spdlog::set_pattern("%^%T   %-8l%$   %-50v         %s:%#");
    spdlog::set_level(spdlog::level::info);

    SPDLOG_INFO("Logging is initialized.");
}