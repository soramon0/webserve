#pragma once

#include "../config/config.hpp"
#include "logger/log.hpp"
#include "../../include/common.h"

SOCKET createSocket(int id, Config& config);