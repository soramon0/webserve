#pragma once

#include <cstdlib>
#include <string>

size_t countDigits(long n);

void reportError(const std::string &src, size_t row, size_t start,
                 const std::string &msg);

std::string strToLower(const std::string &str);
std::string strToUpper(const std::string &str);
