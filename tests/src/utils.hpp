#pragma once
#include <cassert>
#include <string>

void assertStrEquals(const std::string &wanted, const std::string &got,
                     const std::string &msg);

void assertStrEquals(const std::string &wanted, const std::string &got);
