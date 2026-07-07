#ifndef CGI_UTILS
#define CGI_UTILS

#include "CgiHandler.hpp"
#include <vector>

void splitQueryString(const StringView& uri, std::string& path, std::string& query_string);
std::string toHttpEnvName(const std::string& key);
char **vectorToEnvp(const std::vector<std::string>& vect);
void freeEnvp(char **envp);
void close_wrapper(int& fd);

#endif