#include "shared_config.hpp"
#include <algorithm>

static void appendUnique(std::vector<std::string> &dst,
                         const std::vector<std::string> &src) {
  for (size_t i = 0; i < src.size(); ++i) {
    if (std::find(dst.begin(), dst.end(), src[i]) == dst.end())
      dst.push_back(src[i]);
  }
}

SharedConfig SharedConfig::mergeInherited(const SharedConfig &parent,
                                          const SharedConfig &child) {
  SharedConfig out = parent;

  if (!child.root.empty())
    out.root = child.root;

  if (!child.index.empty())
    out.index = child.index;

  if (child.autoindex != SharedConfig::INDEX_UNSET)
    out.autoindex = child.autoindex;

  for (std::map<int, std::string>::const_iterator it = child.error_page.begin();
       it != child.error_page.end(); ++it) {
    out.error_page[it->first] = it->second;
  }

  if (child.client_max_body_size != 0)
    out.client_max_body_size = child.client_max_body_size;

  if (!child.access_log.empty())
    out.access_log = child.access_log;

  if (!child.upload_store.empty())
    out.upload_store = child.upload_store;

  for (mimetype_map::const_iterator it = child.types.begin();
       it != child.types.end(); ++it) {
    appendUnique(out.types[it->first], it->second);
  }

  for (std::map<std::string, std::string>::const_iterator it =
           child.cgi_pass.begin();
       it != child.cgi_pass.end(); ++it) {
    out.cgi_pass[it->first] = it->second;
  }

  return out;
}
