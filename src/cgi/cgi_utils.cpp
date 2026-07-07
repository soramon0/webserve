#include "cgi_utils.hpp"

static size_t findChar(const StringView& sv, char c)
{
	for (size_t pos = 0; pos < sv.length(); pos++)
	{
		if (c == sv.data()[pos])
			return (pos);
	}
	return (sv.length());
}

void splitQueryString(const StringView& uri, std::string& path, std::string& query_string)
{
	size_t pos = findChar(uri, '?');
	path = std::string(uri.data(), pos);
	if (pos == uri.length())
	{
		query_string = "";
		return ;
	}
	else
	{
		query_string = std::string(uri.data() + pos + 1, uri.length() - pos - 1);
		return ;
	}
}

std::string toHttpEnvName(const std::string& key)
{
	std::string env_name = key;

	for (size_t i = 0; i < key.length(); i++)
	{
		if (key[i] == '-')
			env_name[i] = '_';
		else
			env_name[i] = std::toupper(static_cast<unsigned char>(key[i]));
	}
	env_name = "HTTP_" + env_name;
	return (env_name);
}