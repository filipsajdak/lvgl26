// Fuzz target for the JSON reader (include/lv/json.hpp).
//
// The reader is plain C++, so this target is built with Clang and libFuzzer:
//   clang++ -std=c++23 -g -O1 -fsanitize=fuzzer,address,undefined -Iinclude
//       fuzz/json_reader.cpp -o build/fuzz/json_reader     (one command line)
//   build/fuzz/json_reader fuzz/corpus/json -max_total_time=60
//
// It parses the input and, if it is accepted, walks every value with every
// accessor. Beyond "no crash, no sanitizer report" it checks that what the reader
// hands out stays inside the input and that its counts agree with each other.

#include <lv/json.hpp>

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace {

using lv::json::kind;
using lv::json::value;

// Stops the fuzzer with a crash when a property of the reader does not hold.
void require(bool ok) {
  if (!ok) __builtin_trap();
}

// A view the reader returned must lie inside the document it was made from.
void require_inside(std::string_view part, std::string_view whole) {
  if (part.empty()) return;
  require(part.data() >= whole.data());
  require(part.data() + part.size() <= whole.data() + whole.size());
}

void walk(value v, std::string_view document, int depth);

void walk_object(value v, std::string_view document, int depth) {
  const std::size_t n = v.size();
  for (std::size_t i = 0; i < n; ++i) {
    const auto m = v.member_at(i);
    require_inside(m.key, document);
    require_inside(m.text, document);
    require(!m.text.empty());             // every member has a value
    require(static_cast<bool>(v[m.key])); // and can be found by its key
    walk(value{m.text}, document, depth + 1);
  }
  require(v.member_at(n).text.empty());   // one past the end is empty
}

void walk_array(value v, std::string_view document, int depth) {
  const std::size_t n = v.size();
  for (std::size_t i = 0; i < n; ++i) {
    const value element = v.at(i);
    require(static_cast<bool>(element));
    require_inside(element.text(), document);
    walk(element, document, depth + 1);
  }
  require(!v.at(n));
}

void walk(value v, std::string_view document, int depth) {
  require(depth <= 65);  // the reader refuses deeper nesting
  require_inside(v.text(), document);
  switch (v.type()) {
    case kind::object: walk_object(v, document, depth); break;
    case kind::array: walk_array(v, document, depth); break;
    case kind::string: require_inside(v.string(), document); break;
    case kind::number: (void)v.number(); break;
    case kind::boolean: (void)v.boolean(); break;
    default: break;
  }
  // Accessors of the wrong kind must be harmless on every value.
  (void)v.string();
  (void)v.number();
  (void)v.size();
  (void)v["key"];
  (void)v.at(0);
}

}  // namespace

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
  const std::string_view document(reinterpret_cast<const char*>(data), size);
  if (const auto parsed = value::parse(document)) walk(*parsed, document, 0);
  return 0;
}
