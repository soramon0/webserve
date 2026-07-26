#ifndef CGI_DISPATCH_INFO_HPP
#define CGI_DISPATCH_INFO_HPP

#include <string>

struct CgiDispatchInfo
{
	std::string script_path;
	std::string path_info;
	std::string interpreter_path;
	std::string server_name;
	std::string server_port;
	std::string resolved_root;
};

#endif
