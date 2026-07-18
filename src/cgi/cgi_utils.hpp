#ifndef CGI_UTILS
#define CGI_UTILS

#include "CgiHandler.hpp"
#include <vector>
class Client;

void splitQueryString(const StringView &uri, std::string &path, std::string &query_string);
std::string toHttpEnvName(const std::string &key);
char **vectorToEnvp(const std::vector<std::string> &vect);
void freeEnvp(char **envp);
void close_wrapper(int &fd);
bool resolveScriptPath(const std::string &root, const std::string &uri_path,
					   std::string &script_path, std::string &path_info);
bool lookupInterpreter(const std::map<std::string, std::string> &cgi_pass,
					   const std::string &script_path, std::string &interpreter_path);

void resolveServerVars(const Client *cl, std::string &server_name, std::string &server_port);
#endif