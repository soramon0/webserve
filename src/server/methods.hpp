#pragma once
#include "Client.hpp"
#include <dirent.h>

void handleGet(Client* cl);
void handlePost(Client* cl);
void handleDelete(Client* cl);