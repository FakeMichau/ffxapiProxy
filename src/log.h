#pragma once
#include <iostream>
#include <chrono>
#include <cstdint>
#include <string>
#include <source_location>

std::string getCurrentTimeFormatted();
void log(const std::string& log);
template<typename T>
std::string to_string(const T& value) {
    if constexpr (std::is_arithmetic_v<T>) {
        return std::to_string(value);
    } else {
        return std::string(value);
    }
}
template<typename... Args>
void log(const std::string& formatStr, Args&&... args) {
    log(std::vformat(formatStr, std::make_format_args(to_string(std::forward<Args>(args))...)));
}
void prepareLogging(std::optional<std::string> fileName);
void closeLogging();