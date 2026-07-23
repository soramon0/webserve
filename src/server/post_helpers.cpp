#include "post_helpers.hpp"
#include <sys/stat.h>
#include <cerrno>

bool hasParentDirTraversal(const std::string &path)
{
	size_t pos = 0;
	while (pos <= path.length())
	{
		size_t next_slash = path.find('/', pos);
		std::string segment;
		if (next_slash == std::string::npos)
			segment = path.substr(pos);
		else
			segment = path.substr(pos, next_slash - pos);
		if (segment == "..")
			return (true);
		if (next_slash == std::string::npos)
			break;
		pos = next_slash + 1;
	}
	return (false);
}
bool validateParentDir(const std::string &parent_path, HttpStatus::Code &out_status)
{
	struct stat st;
	if (stat(parent_path.c_str(), &st) == -1)
	{
		if (errno == ENOENT)
			out_status = HttpStatus::NOT_FOUND;
		else if (errno == EACCES)
			out_status = HttpStatus::FORBIDDEN;
		else
			out_status = HttpStatus::INTERNAL_SERVER_ERROR;
		return (false);
	}
	else if (!S_ISDIR(st.st_mode))
	{
		out_status = HttpStatus::NOT_FOUND;
		return (false);
	}
	return (true);
}

bool checkTargetPath(const std::string& target_path, HttpStatus::Code& out_status, bool& existed)
{
	struct stat st;
	if (stat(target_path.c_str(), &st) == 0)
	{
		if (S_ISDIR(st.st_mode))
		{
			out_status = HttpStatus::CONFLICT;
			return (false);
		}
		existed = true;
	}
	else if (errno != ENOENT)
	{
		out_status = HttpStatus::INTERNAL_SERVER_ERROR;
		return (false);
	}
	return (true);
}