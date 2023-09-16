#ifndef VE001_LOGGING_H
#define VE001_LOGGING_H

#include <memory>
#include <spdlog/logger.h>

namespace ve001 {

extern std::shared_ptr<spdlog::logger> logger;

}

#endif