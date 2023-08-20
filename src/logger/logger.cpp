#include "logger.h"

#include <memory>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#define VE001_LOG_TO_CONSOLE
#define VE001_LOG_TO_FILE

#if !defined(VE001_LOG_TO_CONSOLE) && !defined(VE001_LOG_TO_FILE)

#error "logs must be written to file, console or both"

#endif

std::shared_ptr<spdlog::logger> ve001::logger(std::make_shared<spdlog::logger>(
    "logger",
    spdlog::sinks_init_list{
#ifdef VE001_LOG_TO_CONSOLE
    #ifndef WINDOWS
        std::make_shared<spdlog::sinks::ansicolor_stdout_sink_mt>(),
    #else
        std::make_shared<spdlog::sinks::wincolor_stdout_sink_mt>().
    #endif
#endif
#ifdef VE001_LOG_TO_FILE
    std::make_shared<spdlog::sinks::rotating_file_sink_mt>("ve001_log", 4096, 1)
#endif
    })
);