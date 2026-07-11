#include "methods.hpp"
#include <algorithm>
#include <sys/stat.h>
#include <fstream>

void handleGet(Client* cl) {
	HttpRequest* req = cl->machine.getRequest();
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
		std::vector<std::string>& indexes = cl->location->shared_config->index;
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
		// if still a directory after checking indexes
		struct stat check;
		if (stat(file_path.c_str(), &check) == -1 || S_ISDIR(check.st_mode))
		{
			req->status = HttpStatus::FORBIDDEN;
			return;
		}
	}

	// next : open file -> read it in a string -> send response
	std::ifstream file(file_path.c_str(), std::ios::binary);// opens file (to avoid system translation)
	std::string body((std::istreambuf_iterator<char>(file)),
		std::istreambuf_iterator<char>()); // reads all of it into "body"

	cl->response_body = body;
	cl->file_path = file_path;
	req->status = HttpStatus::OK;
}

void handlePost(Client* cl)
{
	cl->machine.getRequest()->status = HttpStatus::NOT_IMPLEMENTED;
}

void handleDelete(Client* cl)
{
	cl->machine.getRequest()->status = HttpStatus::NOT_IMPLEMENTED;
}