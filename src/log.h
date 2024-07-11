#pragma once
#include <iostream>
#include <chrono>
#include <cstdint>
#include <string>
#include <source_location>

std::string getCurrentTimeFormatted();
void log(const std::string& log);
void prepareLogging(std::string fileName);
void closeLogging();