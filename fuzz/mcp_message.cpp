// Fuzz target for the MCP message handler (include/lv/mcp.hpp).
//
// The handler needs reflection, so this is a GCC program like runtime_document.cpp:
// it reads one input file, which is how AFL++ drives it (fuzz/afl.sh mcp_message).
//
// The input is a session: one message per line. Whatever the lines are, every
// answer must be one line of well-formed JSON with "jsonrpc" in it, nothing may
// crash or trip a sanitizer, and no file may be written: the server under test
// has write_files off, so a fuzzer cannot reach the file system.

#include <lv/lv.hpp>
#include <lv/mcp.hpp>

#include <cstdio>
#include <string>
#include <string_view>

namespace {

using server = lv::mcp::server<"obj", "label", "button", "arc", "bar", "slider", "switch",
                               "checkbox", "textarea", "dropdown", "roller">;

void require(bool ok) {
  if (!ok) __builtin_trap();
}

std::string read_file(const char* path) {
  std::string text;
  if (std::FILE* f = std::fopen(path, "rb")) {
    char buffer[4096];
    for (std::size_t n; (n = std::fread(buffer, 1, sizeof buffer, f)) > 0;) text.append(buffer, n);
    std::fclose(f);
  }
  return text;
}

void answer(server& agent, std::string_view message) {
  const std::string reply = agent.handle(message);
  if (reply.empty()) return;  // a notification
  require(reply.find('\n') == std::string::npos);
  const auto parsed = lv::json::value::parse(reply);
  require(parsed.has_value());
  require((*parsed)["jsonrpc"].string() == "2.0");
  require(static_cast<bool>((*parsed)["result"]) != static_cast<bool>((*parsed)["error"]));
}

void run(std::string_view input) {
  const auto screen = lv::screen_active();
  const auto before = lv_obj_get_child_count(screen.raw());
  {
    server agent{screen, {.write_files = false}};
    while (!input.empty()) {
      const auto end = input.find('\n');
      answer(agent, input.substr(0, end));
      input.remove_prefix(end == std::string_view::npos ? input.size() : end + 1);
    }
  }
  require(lv_obj_get_child_count(screen.raw()) == before);  // the screen goes with the server
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2) {
    std::printf("usage: mcp_message <input file>\n");
    return 2;
  }
  const std::string input = read_file(argv[1]);
  lv_init();
  lv_test_display_create(320, 240);
  run(input);
  lv_deinit();
  return 0;
}
