#pragma once

#include "Logger.h"

#define LOG_INFO(cat, fmt, ...) \
    tme::log::Logger::Log(tme::log::LogLevel::Info, cat, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define LOG_WARN(cat, fmt, ...) \
    tme::log::Logger::Log(tme::log::LogLevel::Warning, cat, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define LOG_ERROR(cat, fmt, ...) \
    tme::log::Logger::Log(tme::log::LogLevel::Error, cat, __FILE__, __LINE__, fmt, ##__VA_ARGS__)