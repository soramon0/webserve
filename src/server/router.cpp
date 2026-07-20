#include "router.hpp"
#include "methods.hpp"

Location* matchLocation(std::map<std::string, Location>& locations, const std::string& uri)
{
    std::map<std::string, Location>::iterator it = locations.begin();
    size_t best = 0;
    Location* bestLoc = NULL;

    while (it != locations.end())
    {
        std::string prefix = it->first;
        size_t pre_size = prefix.size();
        if (uri.substr(0, pre_size) == prefix && best < pre_size)
        {
            best = pre_size;
            bestLoc = &it->second;
        }
        ++it;
    }
    return bestLoc;
}
void      processRequest(Client* c)
{
	HttpRequest* req = c->machine.getRequest();
	std::string uri(req->uri.data(), req->uri.length());
	Location* loc = matchLocation(c->srv->locations, uri);

	if (loc == NULL)
	{
		req->status = HttpStatus::NOT_FOUND;
		return ;
	}
	if (loc->return_rule != NULL)
	{
		req->status = HttpStatus(loc->return_rule->code);
		c->redirect_url = loc->return_rule->url;
		return ;
	}
	c->location = loc;

	if (req->status == HttpStatus::METHOD_NOT_ALLOWED)
    	return;

	// route to the right method
	if (req->method == HttpMethod::GET)
		handleGet(c);
	else if (req->method == HttpMethod::POST)
		handlePost(c);
	else if (req->method == HttpMethod::DELETE)
		handleDelete(c);
	else
		req->status = HttpStatus::METHOD_NOT_ALLOWED;
}
