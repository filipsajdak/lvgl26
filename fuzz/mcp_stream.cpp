// Fuzz target for the stream an MCP server reads (serve() in include/lv/mcp.hpp).
//
// The handler needs reflection, so this is a GCC program like mcp_message.cpp: it
// reads one input file, which is how AFL++ drives it (fuzz/afl.sh mcp_stream).
//
// The input is any bytes, read as serve() reads stdin, with a small line limit so
// that a fuzzer reaches it. Whatever the bytes are:
// - the reader never keeps more than the limit of a line, and finds the lines that
//   splitting the input on line feeds finds, each too long exactly when it is longer
//   than the limit;
// - every answer is one line of well-formed JSON in well-formed UTF-8, and a line
//   too long gets the one error for it;
// - nothing crashes or trips a sanitizer.

#include <lv/lv.hpp>
#include <lv/mcp.hpp>

#include <cstdio>
#include <string>
#include <string_view>

namespace {

using server = lv::mcp::server<"obj", "label", "button", "arc">;
using lv::mcp::detail::line_status;

// Small, so that a fuzzer's input has room for a line that is too long.
constexpr std::size_t line_limit = 256;

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

// The bytes of a text, one at a time, as std::fgetc gives those of a file. It views
// the input, which outlives it in run().
struct bytes_of {
  std::string_view rest;
  int operator()() {
    if (rest.empty()) return EOF;
    const auto c = static_cast<unsigned char>(rest.front());
    rest.remove_prefix(1);
    return c;
  }
};

// The next line of `input` as splitting on line feeds finds it, taken off `input`.
std::string_view take_line(std::string_view& input) {
  const auto end = input.find('\n');
  const auto line = input.substr(0, end);
  input.remove_prefix(end == std::string_view::npos ? input.size() : end + 1);
  return line;
}

// The reader agrees with a split of the whole input, and keeps no more than the limit.
std::size_t check_reader(std::string_view input) {
  bytes_of next{input};
  std::string line;
  std::size_t too_long = 0;
  for (auto status = lv::mcp::detail::read_line(next, line, line_limit); status != line_status::end;
       status = lv::mcp::detail::read_line(next, line, line_limit)) {
    const auto expected = take_line(input);
    require(line.size() <= line_limit);
    require((status == line_status::too_long) == (expected.size() > line_limit));
    require(status == line_status::too_long || line == expected);
    if (status == line_status::too_long) ++too_long;
  }
  require(input.empty());
  return too_long;
}

// One answer: a line of well-formed JSON in UTF-8. Whether it is the error for a
// line that is too long.
bool check_answer(std::string_view reply) {
  require(reply.find('\n') == std::string_view::npos);
  require(lv::json::detail::well_formed(reply));
  const auto parsed = lv::json::value::parse(reply);
  require(parsed.has_value());
  require((*parsed)["jsonrpc"].string() == "2.0");
  require(static_cast<bool>((*parsed)["result"]) != static_cast<bool>((*parsed)["error"]));
  return reply == lv::mcp::detail::too_long(line_limit);
}

void run(std::string_view input) {
  const std::size_t too_long = check_reader(input);
  server agent{lv::screen_active()};
  auto handle = [&agent](std::string_view message) { return agent.handle(message); };
  bytes_of next{input};
  std::size_t refused = 0;
  auto write = [&refused](std::string_view reply) { if (check_answer(reply)) ++refused; };
  lv::mcp::detail::serve_lines(handle, next, write, line_limit);
  require(refused == too_long);
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2) {
    std::printf("usage: mcp_stream <input file>\n");
    return 2;
  }
  const std::string input = read_file(argv[1]);
  lv_init();
  lv_test_display_create(320, 240);
  run(input);
  lv_deinit();
  return 0;
}
