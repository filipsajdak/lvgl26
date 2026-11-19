// Prints the public names of lvgl26, one for each line: every member of namespace lv
// and of the namespaces inside it, apart from lv::detail. Reflection lists them, so a
// new public name is on the list without anyone adding it.
//
// tools/editor/inventory.py compares the list with the editor stubs (tools/editor/stubs)
// and with tests/editor/api_uses.cpp. The test editor_names runs both.

#include <lv/host.hpp>
#include <lv/lv.hpp>
#include <lv/mcp.hpp>
#include <lv/one_of.hpp>
#include <lv/png.hpp>
#include <lv/pointer.hpp>
#include <lv/signal.hpp>
#include <lv/signal_type.hpp>
#include <lv/ui_scheduler.hpp>

#include <cstdio>
#include <meta>
#include <string>
#include <string_view>
#include <vector>

namespace {

consteval bool is_private_namespace(std::string_view name) {
  return name == "detail";
}

// The names of the members of `space`, each with `prefix` before it.
consteval void collect(std::vector<std::string>& out, std::meta::info space, const std::string& prefix) {
  for (auto member : std::meta::members_of(space, std::meta::access_context::unchecked())) {
    if (!std::meta::has_identifier(member)) continue;
    const std::string_view name = std::meta::identifier_of(member);
    if (!std::meta::is_namespace(member)) {
      out.push_back(prefix + std::string{name});
    } else if (!is_private_namespace(name)) {
      collect(out, member, prefix + std::string{name} + "::");
    }
  }
}

consteval std::string_view public_names() {
  std::vector<std::string> names;
  collect(names, ^^lv, "lv::");
  std::string text;
  for (const auto& name : names) text += name + "\n";
  return std::define_static_string(text);
}

}  // namespace

int main() {
  constexpr std::string_view names = public_names();
  std::fwrite(names.data(), 1, names.size(), stdout);
}
