#include "methods.hpp"
#include "cgi/cgi_utils.hpp"
#include <algorithm>
#include <sys/stat.h>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <fcntl.h>
#include <cerrno>

std::string getFilePath(Client* cl) {
	HttpRequest* req = cl->machine.getRequest();
	std::string uri(req->uri.data(), req->uri.length());
	std::string uri_suffix = uri.substr(cl->location->path.size());
	std::string file_path = cl->location->shared_config->root + "/" + uri_suffix;

	return file_path;
}

HttpStatus::Code getHttpStatusError() {
	if (errno == ENOENT) return HttpStatus::NOT_FOUND;
	if (errno == EACCES) return HttpStatus::FORBIDDEN;
	return HttpStatus::INTERNAL_SERVER_ERROR;
}

// TODO : .. : reject in get & delete: qlbi ktr
void handleGet(Client* cl) {
	if (tryDispatchCgi(cl, *cl->cgiManager))
		return;
	HttpRequest* req = cl->machine.getRequest();
	std::string file_path = getFilePath(cl);
	Logger::debug("uri is : %s", file_path.c_str());

	struct stat file_stat;
	if (stat(file_path.c_str(), &file_stat) == -1)
	{
		Logger::debug("This uri doesn't exist");
		req->status = getHttpStatusError();
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
					req->status = getHttpStatusError();
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

	cl->response.chunked = 1;
	Logger::debug("the size stat give is : %zu", cl->response.file_size);
	cl->response.file_fd = open(file_path.c_str(), O_RDONLY);
	if (cl->response.file_fd == -1) {
		req->status = getHttpStatusError();
		return;
	}
	mimetype_map empty_types;
	mimetype_map& types = (cl->location && cl->location->shared_config) 
		? cl->location->shared_config->types
		: empty_types;
	cl->response.buildHeaders(*req, getContentType(file_path, types));
	req->status = HttpStatus::OK;
}
// TODO : add Date header to DELETE response
/**
 * What Nginx does

Nginx normalizes the URI and ensures the resulting
filesystem path stays inside the configured root (or alias)
directory. Requests containing unsafe traversal sequences
are rejected before filesystem operations occur.

decode...
 */
void handleDelete(Client* cl) {
	if (tryDispatchCgi(cl, *cl->cgiManager))
		return;
	HttpRequest* req = cl->machine.getRequest();
	std::string file_path = getFilePath(cl);

	Logger::info("uri is : %s", file_path.c_str());

	// check the file existance
	struct stat file_stat;
	if (stat(file_path.c_str(), &file_stat) == -1)
	{
		Logger::info("DELETE: the path is not  found");
		req->status = getHttpStatusError();
		return;
	}

	if (std::remove(file_path.c_str()))
	{
		Logger::info("DELETE : can't delete this file/dir '%s'", file_path.c_str());
		req->status = getHttpStatusError();
		return ;
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
	struct stat st;
	size_t pos = target_path.find_last_of('/');
	std::string parent_path = target_path.substr(0, pos);
	if (stat(parent_path.c_str(), &st) == -1)
	{
		if (errno == ENOENT)
			req->status = HttpStatus::NOT_FOUND;
		else if (errno == EACCES)
			req->status = HttpStatus::FORBIDDEN;
		else
			req->status = HttpStatus::INTERNAL_SERVER_ERROR;
		return;
	}
	else if (!S_ISDIR(st.st_mode))
	{
		req->status = HttpStatus::NOT_FOUND;
		return;
	}
	std::ofstream outfile(target_path.c_str(), std::ios::binary | std::ios::trunc);
	if (!outfile.is_open())
	{
		req->status = HttpStatus::INTERNAL_SERVER_ERROR;
		return;
	}
	RequestBody::ReadResult res;
	do
	{
		res = req->body.read();
		if (res.status == RequestBody::READ_ERROR)
		{
			outfile.close();
			std::remove(target_path.c_str());
			req->body.resetReader();
			req->status = HttpStatus::INTERNAL_SERVER_ERROR;
			return;
		}
		if (res.block)
		{
			outfile.write(reinterpret_cast<const char *>(res.block->getBuffer()), res.block->consumed());
			if (outfile.fail())
			{
				outfile.close();
				std::remove(target_path.c_str());
				req->body.resetReader();
				req->status = HttpStatus::INTERNAL_SERVER_ERROR;
				return;
			}
		}
	} while (res.status != RequestBody::READ_DONE);
	req->body.resetReader();
	outfile.close();
	req->status = HttpStatus::CREATED;	
}
