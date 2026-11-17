#pragma once

// Editor stub of lv/json.hpp: a JSON reader. The real one also runs in the compiler; the
// editor does not work a value out, so here its functions are not constexpr.

#include <cstddef>
#include <iterator>
#include <optional>
#include <string_view>

namespace lv::json {

/// What a JSON value is.
enum class kind { invalid, null, boolean, number, string, array, object };

class value;
template <class Item>
class items;

/// One member of an object: its key, and the text of its value.
struct member {
  std::string_view key;
  std::string_view text;
};

/// A view of one JSON value inside a document. It allocates nothing.
class value {
 public:
  constexpr value() = default;
  constexpr explicit value(std::string_view text) : text_(text) {}

  /// The document as a value, or nothing when it is not JSON.
  static std::optional<value> parse(std::string_view doc);

  /// The text of the value, as the document has it.
  constexpr std::string_view text() const { return text_; }
  kind type() const;
  bool is(kind k) const;
  /// The text of a string, without the quotes. Escapes are left as they are.
  std::string_view string() const;
  bool boolean() const;
  double number() const;
  /// The number of elements of an array, or of members of an object.
  std::size_t size() const;
  /// The members of an object, walked once in document order; none for anything else.
  items<member> members() const;
  /// The elements of an array, walked once in order; none for anything else.
  items<value> elements() const;
  member member_at(std::size_t index) const;
  value at(std::size_t index) const;
  /// The member of an object with this key, or an invalid value.
  value operator[](std::string_view key) const;
  /// False for a value that is not there.
  explicit operator bool() const;

 private:
  std::string_view text_;
};

/// The members of an object or the elements of an array, for a range-for loop.
template <class Item>
class items {
 public:
  class iterator {
   public:
    using value_type = Item;
    using difference_type = std::ptrdiff_t;
    Item operator*() const { __builtin_unreachable(); }
    iterator& operator++() { __builtin_unreachable(); }
    iterator operator++(int) { __builtin_unreachable(); }
    friend bool operator==(const iterator&, const iterator&) { __builtin_unreachable(); }
    friend bool operator==(const iterator&, std::default_sentinel_t) { __builtin_unreachable(); }
  };
  iterator begin() const { __builtin_unreachable(); }
  std::default_sentinel_t end() const { return {}; }
};

}  // namespace lv::json
