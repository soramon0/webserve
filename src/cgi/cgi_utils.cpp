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