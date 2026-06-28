#pragma once
#include "lib/string_view.hpp"
#include <map>

#ifndef REQUEST_HEADERS_
#define REQUEST_HEADERS_

typedef std::map<StringView, StringView> HStore;

#endif // ! REQUEST_HEADERS_

class Headers {
private:
  HStore store;

public:
  Headers() {};

  StringView get(const StringView &key) const {
    HStore::const_iterator it = store.find(key);

    if (it != store.end()) {
      return it->second;
    }

    return StringView();
  }

  void set(const StringView &key, const StringView &value) {
    std::pair<HStore::iterator, bool> result =
        store.insert(std::make_pair(key, value));

    if (!result.second) {
      // key already exists
      // TODO: may need to appened value
      result.first->second = value;
    }
  }
}
