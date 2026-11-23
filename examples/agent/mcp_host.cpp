// A headless LVGL program that an agent can drive over the Model Context Protocol.
//
//   mcp_host                      answers requests on stdin, one JSON-RPC message per line
//   mcp_host session.jsonl        answers the requests of a file, then ends
//   mcp_host session.jsonl shots  the same, and lets the client write screenshots
//                                 into the directory "shots"
//
// Without a directory the client gets its screenshots as images and cannot write
// files. With one it can make new *.png files there, and nothing else.
//
// To try it by hand:
//   echo '{"jsonrpc":"2.0","id":1,"method":"tools/list"}' | mcp_host
// To give it to an agent, register the program as an MCP server with the stdio
// transport. The agent can then load a screen, change it, read the widget tree
// and take screenshots: see lv/mcp.hpp.
//
// The display has no window and no panel: LVGL draws into memory. Screenshots are
// drawn on request.

#include <lv/lv.hpp>
#include <lv/mcp.hpp>

#include <cstdint>
#include <cstdio>

namespace {

constexpr std::int32_t width  = 320;
constexpr std::int32_t height = 240;

// Where LVGL draws: the display has no panel.
constinit lv::frame<width, height> frame;

}  // namespace

int main(int argc, char** argv) {
  std::FILE* in = argc > 1 ? std::fopen(argv[1], "r") : stdin;
  if (in == nullptr) {
    std::fprintf(stderr, "mcp_host: cannot read %s\n", argv[1]);
    return 2;
  }

  lv::application app;
  lv::display display{app, frame};

  {
    lv::mcp::server<"obj", "label", "button", "arc", "bar", "slider", "switch", "checkbox"> agent{
      display.screen(),
      {.name = "lvgl26 example host", .screenshot_directory = argc > 2 ? argv[2] : ""}
    };
    agent.serve(in, stdout);
  }

  if (in != stdin) std::fclose(in);
  return 0;
}
