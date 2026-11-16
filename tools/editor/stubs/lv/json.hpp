#pragma once

// Editor stub of lv/json.hpp: a JSON reader. The real one also runs in the compiler; the
// editor does not work a value out, so here its functions are not constexpr.

#include <cstddef>
#include <optional>
#include <string_view>

namespace lv::json {

/// What a JSON value is.
enum class kind { invalid, null, boolean, number, string, array, object };

class value;

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
  member member_at(std::size_t index) const;
  value at(std::size_t index) const;
  /// The member of an object with this key, or an invalid value.
  value operator[](std::string_view key) const;
  /// False for a value that is not there.
  explicit operator bool() const;

 private:
  std::string_view text_;
};

}  // namespace lv::json
