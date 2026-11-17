// Fuzz target for the MCP message handler (include/lv/mcp.hpp).
//
// The handler needs reflection, so this is a GCC program like runtime_document.cpp:
// it reads one input file, which is how AFL++ drives it (fuzz/afl.sh mcp_message).
//
// The input is a session: one message per line. Whatever the lines are, every
// answer must be one line of well-formed JSON in well-formed UTF-8 with "jsonrpc" in
// it, and nothing may crash or trip a sanitizer.
//
// Files: every line goes to two servers. One is as a host gets it by default and
// may write no files. The other may make screenshots in a directory made for this
// run, next to a second directory it must never reach. The first holds a file
// (taken.png) and a symbolic link into the second (link.png). After the session
// the file is as it was, the link is still a link, the second directory is empty,
// and everything new in the first has the name of a PNG file and stays in the limit.

#include <lv/lv.hpp>
#include <lv/mcp.hpp>

#include <cstdio>
#include <filesystem>
#include <iterator>
#include <random>
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

// The two directories of a run. They go when the run ends.
struct places {
  std::filesystem::path base = std::filesystem::temp_directory_path() /
                               ("lvgl26-mcp-fuzz-" + std::to_string(std::random_device{}()));
  std::filesystem::path shots = base / "shots";
  std::filesystem::path outside = base / "outside";

  places() {
    std::error_code failed;
    require(std::filesystem::create_directory(base, failed));  // false if it was there
    require(std::filesystem::create_directory(shots, failed));
    require(std::filesystem::create_directory(outside, failed));
    std::FILE* taken = std::fopen((shots / "taken.png").c_str(), "wb");
    require(taken != nullptr);
    std::fputs("kept", taken);
    std::fclose(taken);
    std::filesystem::create_symlink(outside / "target.png", shots / "link.png", failed);
    require(!failed);
  }
  places(const places&) = delete;
  places& operator=(const places&) = delete;
  ~places() {
    std::error_code ignored;
    std::filesystem::remove_all(base, ignored);
  }

  // What must hold whatever the client asked for.
  void untouched() const {
    require(read_file((shots / "taken.png").c_str()) == "kept");
    require(std::filesystem::is_symlink(shots / "link.png"));
    require(std::filesystem::is_empty(outside));
    require(std::distance(std::filesystem::directory_iterator{base},
                          std::filesystem::directory_iterator{}) == 2);
    for (const auto& entry : std::filesystem::directory_iterator{shots})
      require(lv::mcp::detail::usable_file_name(entry.path().filename().string()));
    constexpr std::size_t made_here = 2;  // taken.png and link.png
    require(static_cast<std::size_t>(std::distance(std::filesystem::directory_iterator{shots},
                                                   std::filesystem::directory_iterator{})) <=
            made_here + lv::mcp::detail::screenshot_file_limit);
  }
};

void answer(server& agent, std::string_view message) {
  const std::string reply = agent.handle(message);
  if (reply.empty()) return;  // a notification
  require(reply.find('\n') == std::string::npos);
  require(lv::json::detail::well_formed(reply));
  const auto parsed = lv::json::value::parse(reply);
  require(parsed.has_value());
  require((*parsed)["jsonrpc"].string() == "2.0");
  require(static_cast<bool>((*parsed)["result"]) != static_cast<bool>((*parsed)["error"]));
}

void run(std::string_view input) {
  const auto screen = lv::screen_active();
  const auto before = lv_obj_get_child_count(screen.raw());
  const places files;
  const std::string shots = files.shots.string();
  {
    server careful{screen};
    server agent{screen, {.screenshot_directory = shots}};
    while (!input.empty()) {
      const auto end = input.find('\n');
      answer(careful, input.substr(0, end));
      answer(agent, input.substr(0, end));
      input.remove_prefix(end == std::string_view::npos ? input.size() : end + 1);
    }
  }
  files.untouched();
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
