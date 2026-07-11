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

	// next : open file -> read it in a string -> send response
	std::ifstream file(file_path.c_str(), std::ios::binary);// opens file (to avoid system translation)
	std::string body((std::istreambuf_iterator<char>(file)),
		std::istreambuf_iterator<char>()); // reads all of it into "body"

	cl->response_body = body;
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