#include "CgiResponse.hpp"
#include <cctype>
#include <vector>

static size_t findBoundary(const std::string& cgi_output, size_t& separator_width)
{
	size_t crlf_pos = cgi_output.find("\r\n\r\n");
	size_t lf_pos = cgi_output.find("\n\n");

	if (crlf_pos == std::string::npos && lf_pos == std::string::npos)
		return (std::string::npos);
	if (crlf_pos == std::string::npos || (lf_pos != std::string::npos && lf_pos < crlf_pos))
	{
		separator_width = 2;
		return (lf_pos);
	}
	separator_width = 4;
	return (crlf_pos);
}

static void trimSpaces(std::string& s)
{
	size_t start = 0;
	while (start < s.length() && (s[start] == ' ' || s[start] == '\t'))
		start++;
	if (start == s.length())
	{
		s.clear();
		return ;
	}
	size_t end = s.length() - 1;
	while (end > start && (s[end] == ' ' || s[end] == '\t'))
		end--;
	s = s.substr(start, end - start + 1);
}

static void toLowerAscii(std::string& s)
{
	for (size_t i = 0; i < s.length(); i++)
		s[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(s[i])));
}

static void splitHeaderLines(const std::string& header_block, std::vector<std::string>& lines)
{
	size_t pos = 0;
	while (pos < header_block.length())
	{
		size_t line_end = header_block.find('\n', pos);
		size_t next_pos;
		size_t content_end;

		if (line_end == std::string::npos)
		{
			content_end = header_block.length();
			next_pos = header_block.length();
		}
		else
		{
			content_end = line_end;
			next_pos = line_end + 1;
		}
		if (content_end > pos && header_block[content_end - 1] == '\r')
			content_end--;
		if (content_end > pos)
			lines.push_back(header_block.substr(pos, content_end - pos));
		pos = next_pos;
	}
}

static void parseHeaderLine(const std::string& line, CgiResponse& response)
{
	size_t colon = line.find(":");
	if (colon == std::string::npos)
		return ;
	std::string key = line.substr(0, colon);
	std::string value = line.substr(colon + 1);

	trimSpaces(key);
	trimSpaces(value);
	toLowerAscii(key);

	if (key == "status")
	{
		if (value.length() >= 3 && std::isdigit(static_cast<unsigned char>(value[0]))
								&& std::isdigit(static_cast<unsigned char>(value[1]))
								&& std::isdigit(static_cast<unsigned char>(value[2])))
		{
			int code = (value[0] - '0') * 100 + (value[1] - '0') * 10 + (value[2] - '0');
			response.status_code = code;
		}
		return ;
	}
	response.headers.insert(std::make_pair(key, value));
}


CgiResponse parseCgiOutput(const std::string& cgi_output)
{
	CgiResponse response;
	response.status_code = 200;

	size_t separator_width = 0;
	size_t boundary = findBoundary(cgi_output, separator_width);
	if (boundary == std::string::npos)
	{
		response.status_code = 502;
		return (response);
	}
	std::string header_block = cgi_output.substr(0, boundary);
	response.body = cgi_output.substr(boundary + separator_width);
	std::vector<std::string> lines;
	splitHeaderLines(header_block, lines);
	for (size_t i = 0; i < lines.size(); i++)
		parseHeaderLine(lines[i], response);
	return (response);
}