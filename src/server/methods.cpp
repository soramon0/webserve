#include "methods.hpp"
#include <algorithm>
#include <sys/stat.h>
#include <fstream>
#include <sstream>
#include <cstdio> // remove
#include <fcntl.h>

std::string getFilePath(Client* cl) {
	HttpRequest* req = cl->machine.getRequest();
	std::string uri(req->uri.data(), req->uri.length());
	std::string uri_suffix = uri.substr(cl->location->path.size());
	std::string file_path = cl->location->shared_config->root + "/" + uri_suffix;

	return file_path;
}
// TODO : fix the autoindex prb i.e : on click it routs to 404

void handleGet(Client* cl) {
	HttpRequest* req = cl->machine.getRequest();
	std::string file_path = getFilePath(cl);
	Logger::info("uri is : %s", file_path.c_str());

	struct stat file_stat;
	if (stat(file_path.c_str(), &file_stat) == -1)
	{
		Logger::info("This uri doesn't exist");
		req->status = HttpStatus::NOT_FOUND;
		return;
	}
	cl->response.file_size = file_stat.st_size;

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
		// is indx found is a directory after checking indexes
		struct stat check;
		if (stat(file_path.c_str(), &check) == -1 || S_ISDIR(check.st_mode))
		{
			if (cl->location->shared_config->autoindex == SharedConfig::INDEX_ON)
			{
				// generate directory listing
				DIR* dir = opendir(file_path.c_str());
				if (dir == NULL) {
					req->status = HttpStatus::FORBIDDEN;
					return;
				}
				std::ostringstream listing;
				listing << "<html><body><h1>Index of " << req->uri << "</h1><ul>";
				struct dirent* entry;
				while ((entry = readdir(dir)) != NULL)
				{
					std::string name = entry->d_name;
					if (name == "." || name == "..")
						continue;
					listing << "<li><a href=\"" << req->uri << "/" << name << "\">"
							<< name << "</a></li>";
				}
				closedir(dir);
				listing << "</ul></body></html>";
				cl->response.body = listing.str();
				cl->file_path = "";
				req->status = HttpStatus::OK;
				return;
			}
			req->status = HttpStatus::FORBIDDEN;
			return;
		}
		cl->response.file_size = check.st_size;
	}

	//try again
	// // next : open file -> read it in a string -> send response
	cl->response.chunked = 1;
	Logger::debug("the size stat give is : %zu", cl->response.file_size);
	cl->response.file_fd = open(file_path.c_str(), O_RDONLY);
	if (cl->response.file_fd == -1) {
		req->status = HttpStatus::FORBIDDEN;
		return;
	}
	mimetype_map empty_types;
	mimetype_map& types = (cl->location && cl->location->shared_config) 
		? cl->location->shared_config->types 
		: empty_types;
	cl->response.buildHeaders(*req, getContentType(file_path, types));
	cl->file_path = file_path;// useless now after this new approach
	req->status = HttpStatus::OK;

	// std::ifstream file(file_path.c_str(), std::ios::binary);// opens file (to avoid system translation)
	// std::string body((std::istreambuf_iterator<char>(file)),
	// 	std::istreambuf_iterator<char>()); // reads all of it into "body"

	// cl->response.body = body;
}

void handleDelete(Client* cl) {
	HttpRequest* req = cl->machine.getRequest();
	std::string file_path = getFilePath(cl);

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
		return ;
	}
	req->status = HttpStatus::NO_CONTENT;// deleted succssesfully
}

void handlePost(Client* cl)
{
	cl->machine.getRequest()->status = HttpStatus::NOT_IMPLEMENTED;
}