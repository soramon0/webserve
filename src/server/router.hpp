#pragma once
#include "../config/server.hpp"
#include "Client.hpp"

Location* matchLocation(std::map<std::string, Location>& locations, const std::string& uri);
void      processRequest(Client* cl);