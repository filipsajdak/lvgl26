// Prints which header of lvgl26 declares each public name, read by reflection from lv/all.hpp:
// a JSON object from each header to the names of namespace lv it declares. A name with
// declarations in several headers (lv::bind) is listed under each.
//
//   lv_headers_reference > docs/reference/headers.json
//
// tools/check_includes.py reads the file, and docs/which-header.md is written from it. The test
// `headers_data` fails when the committed file differs.
#include <lv/all.hpp>

#include <cstdio>
#include <meta>
#include <string>
#include <string_view>
#include <vector>

namespace {

// The header of a declaration, as it is included: "lv/list.hpp". Empty for a file outside
// include/lv, or for one under lv/detail, which a program never includes itself.
consteval std::string_view header_of(std::meta::info m) {
  const std::string_view file = std::meta::source_location_of(m).file_name();
  const auto at = file.rfind("/include/lv/");
  if (at == std::string_view::npos) return {};
  const auto header = file.substr(at + 9);
  if (header == "lv/detail/missing.hpp") return header;
  if (header.starts_with("lv/detail/")) return {};
  return header;
}

// The header of a name that lv/detail/missing.hpp declares as a fallback: its specialization is in
// that header, where reflection does not look.
consteval std::string_view header_of_fallback(std::string_view name) {
  for (const auto& f : lv::detail::fallbacks)
    if (f.name == name) return f.header;
  lv::detail::fail("lv::" + lv::detail::str(name) + " is declared in lv/detail/missing.hpp and has no header in detail::fallbacks.");
  return {};
}

struct entry {
  std::string_view header;
  std::string_view name;
};

// Each public name with its header, once, in the order of declaration. Views of the
// compiler's own strings: a std::string for each would cost the interpreter far more.
consteval std::vector<entry> entries() {
  std::vector<entry> out;
  for (auto m : std::meta::members_of(^^lv, std::meta::access_context::unchecked())) {
    if (!std::meta::has_identifier(m) || std::meta::is_namespace(m)) continue;
    const std::string_view name = std::meta::identifier_of(m);
    if (name.empty() || name[0] == '_') continue;
    auto header = header_of(m);
    if (header.empty()) continue;
    if (header == "lv/detail/missing.hpp") header = header_of_fallback(name);
    bool seen = false;
    for (const auto& e : out) seen = seen || (e.name.size() == name.size() && e.name == name && e.header == header);
    if (!seen) out.push_back({header, name});
  }
  return out;
}

// The headers in alphabetical order, each with its names in the order of declaration.
consteval std::string document() {
  const auto all = entries();
  std::vector<std::string_view> headers;
  for (const auto& e : all) {
    bool seen = false;
    for (const auto h : headers) seen = seen || h == e.header;
    if (!seen) headers.push_back(e.header);
  }
  // In alphabetical order, so that moving a declaration within the headers moves no line.
  for (std::size_t i = 1; i < headers.size(); ++i)
    for (std::size_t j = i; j > 0 && headers[j] < headers[j - 1]; --j) std::swap(headers[j], headers[j - 1]);
  std::string out = "{";
  for (std::size_t i = 0; i < headers.size(); ++i) {
    out += i == 0 ? "\n  \"" : ",\n  \"";
    out.append(headers[i].begin(), headers[i].end());
    out += "\": [";
    bool first = true;
    for (const auto& e : all) {
      if (e.header != headers[i]) continue;
      out += first ? "\"" : ", \"";
      out.append(e.name.begin(), e.name.end());
      out += "\"";
      first = false;
    }
    out += "]";
  }
  return out + "\n}\n";
}

}  // namespace

int main() {
  constexpr const char* text = std::define_static_string(document());
  std::fputs(text, stdout);
}
