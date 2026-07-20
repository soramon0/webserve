#include "methods.hpp"
#include "cgi_utils.hpp"
#include <algorithm>
#include <sys/stat.h>
#include <fstream>
#include <sstream>
#include <cstdio> // remove

void handleGet(Client *cl)
{
	HttpRequest *req = cl->machine.getRequest();
	std::string uri(req->uri.data(), req->uri.length());
	std::string uri_suffix = uri.substr(cl->location->path.size());
	std::string file_path = cl->location->shared_config->root + "/" + uri_suffix;

	Logger::info("uri is : %s", file_path.c_str());

	// check the file existance
	struct stat file_stat;
	if (stat(file_path.c_str(), &file_stat) == -1)
	{
		Logger::info("HERE I CAN NOT FIND the path");
		// file doesn't exist
		req->status = HttpStatus::NOT_FOUND;
		return;
	}
	// durectory handling
	if (S_ISDIR(file_stat.st_mode))
	{
		// try each index file in order
		std::vector<std::string> &indexes = cl->location->shared_config->index;
		for (size_t i = 0; i < indexes.size(); i++)
		{
			std::string index_path = file_path + "/" + indexes[i];
			struct stat index_stat;
			if (stat(index_path.c_str(), &index_stat) == 0)
			{
				file_path = index_path;
				break;
			}
		}
		// if indx found is  a directory after checking indexes
		struct stat check;
		if (stat(file_path.c_str(), &check) == -1 || S_ISDIR(check.st_mode))
		{
			if (cl->location->shared_config->autoindex == SharedConfig::INDEX_ON)
			{
				// generate directory listing
				DIR *dir = opendir(file_path.c_str());
				if (dir == NULL)
				{
					req->status = HttpStatus::FORBIDDEN;
					return;
				}
				std::ostringstream listing;
				listing << "<html><body><h1>Index of " << uri << "</h1><ul>";
				struct dirent *entry;
				while ((entry = readdir(dir)) != NULL)
				{
					std::string name = entry->d_name;
					if (name == "." || name == "..")
						continue;
					listing << "<li><a href=\"" << name << "\">" << name << "</a></li>";
				}
				closedir(dir);
				listing << "</ul></body></html>";
				cl->response_body = listing.str();
				cl->file_path = "";
				req->status = HttpStatus::OK;
				return;
			}
			req->status = HttpStatus::FORBIDDEN;
			return;
		}
	}

	// next : open file -> read it in a string -> send response
	std::ifstream file(file_path.c_str(), std::ios::binary); // opens file (to avoid system translation)
	std::string body((std::istreambuf_iterator<char>(file)),
					 std::istreambuf_iterator<char>()); // reads all of it into "body"

	cl->response_body = body;
	cl->file_path = file_path;
	req->status = HttpStatus::OK;
}

void handleDelete(Client *cl)
{
	HttpRequest *req = cl->machine.getRequest();
	std::string uri(req->uri.data(), req->uri.length());
	std::string uri_suffix = uri.substr(cl->location->path.size());
	std::string file_path = cl->location->shared_config->root + "/" + uri_suffix;
	// TODO: strip the query from the uri string
	Logger::info("uri is : %s", file_path.c_str());

	// check the file existance
	struct stat file_stat;
	if (stat(file_path.c_str(), &file_stat) == -1)
	{
		Logger::info("DELETE: the path is not  found");
		req->status = HttpStatus::NOT_FOUND;
		return;
	}

	if (std::remove(file_path.c_str()))
	{
		Logger::info("DELETE : can't delete this file/dir '%s'", file_path.c_str());
		req->status = HttpStatus::FORBIDDEN;
		return;
	}
	req->status = HttpStatus::NO_CONTENT; // deleted succssesfully
}

static bool hasParentDirTraversal(const std::string &path)
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

void handlePost(Client *cl)
{
	if (tryDispatchCgi(cl, *cl->cgiManager))
		return;
	HttpRequest *req = cl->machine.getRequest();
	std::string uri(req->uri.data(), req->uri.length());
	std::string uri_suffix = uri.substr(cl->location->path.size());
	if (cl->location->shared_config->upload_store.empty()) // if upload is not supported
	{
		req->status = HttpStatus::FORBIDDEN;
		return;
	}
	if (hasParentDirTraversal(uri_suffix))
	{
		req->status = HttpStatus::FORBIDDEN;
		return;
	}
	std::string target_path = cl->location->shared_config->root + "/" +
							  cl->location->shared_config->upload_store + "/" + uri_suffix;
	std::ofstream outfile(target_path.c_str(), std::ios::binary | std::ios::trunc);
	if (!outfile.is_open())
	{
		req->status = HttpStatus::INTERNAL_SERVER_ERROR;
		return;
	}
}
