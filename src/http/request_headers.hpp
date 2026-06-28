#pragma once
#include "lib/string_view.hpp"
#include <map>

class Headers {
private:
  typedef std::multimap<StringView, StringView> HStore;
  HStore store;

public:
  typedef std::pair<HStore::const_iterator, HStore::const_iterator> Range;

  Headers() {};

  /**
   * @brief Retrieves the first value associated with a header key.
   * @param key The HTTP header key to search for. MUST BE STRICTLY LOWERCASE.
   * @return A const pointer to the StringView containing the value,
   * or NULL if the header does not exist.
   */
  const StringView *get(const StringView &key) const {
    HStore::const_iterator it = store.find(key);

    if (it != store.end()) {
      return &(it->second);
    }

    return NULL;
  }

  /**
   * @brief Retrieves all values associated with a header key.
   * * Useful for headers like 'Accept' or 'Set-Cookie' that may appear multiple
   * times. If the key does not exist, range.first will equal range.second.
   * * @param key The HTTP header key to search for. MUST BE STRICTLY LOWERCASE.
   * @return A Range pair of const_iterators to safely loop through the values.
   */
  Range get_all(const StringView &key) const { return store.equal_range(key); }

  /**
   * @brief Inserts a new header key-value pair.
   * If the key already exists, this does not overwrite it; it appends the new
   * value to the internal multimap alongside the existing ones.
   * @param key The header key. The caller MUST lowercase the buffer it points
   * to.
   * @param value The header value.
   */
  void set(const StringView &key, const StringView &value) {
    store.insert(std::make_pair(key, value));
  }

  /**
   * @brief Checks if a specific header exists.
   * @param key The HTTP header key to search for. MUST BE STRICTLY LOWERCASE.
   * @return true if the header is found, false otherwise.
   */
  bool has(const StringView &key) const {
    return store.find(key) != store.end();
  }
};
