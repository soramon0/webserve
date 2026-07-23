#ifndef POST_HELPERS_HPP
#define POST_HELPERS_HPP

#include <string>
#include "http/status_code.hpp"

bool hasParentDirTraversal(const std::string &path);
bool validateParentDir(const std::string& parent_path, HttpStatus::Code& out_status);
bool checkTargetPath(const std::string& target_path, HttpStatus::Code& out_status, bool& existed);

#endif