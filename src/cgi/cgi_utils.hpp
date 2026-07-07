#ifndef CGI_UTILS
#define CGI_UTILS

#include "CgiHandler.hpp"

void splitQueryString(const StringView& uri, std::string& path, std::string& query_string);

#endif