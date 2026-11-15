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
// The display has no window: LVGL draws into memory and the picture is thrown
// away. Screenshots are drawn on request.

#include <lv/lv.hpp>
#include <lv/mcp.hpp>

#include <cstdint>
#include <cstdio>

namespace {

constexpr std::int32_t width = 320;
constexpr std::int32_t height = 240;

// Where LVGL draws. Four bytes cover a pixel of every color depth.
alignas(std::uint32_t) std::uint8_t frame[width * height * 4];

void discard(lv_display_t* display, const lv_area_t*, std::uint8_t*) {
  lv_display_flush_ready(display);
}

}  // namespace

int main(int argc, char** argv) {
  std::FILE* in = argc > 1 ? std::fopen(argv[1], "r") : stdin;
  if (in == nullptr) {
    std::fprintf(stderr, "mcp_host: cannot read %s\n", argv[1]);
    return 2;
  }

  lv_init();
  lv_display_t* display = lv_display_create(width, height);
  lv_display_set_buffers(display, frame, nullptr, sizeof frame, LV_DISPLAY_RENDER_MODE_PARTIAL);
  lv_display_set_flush_cb(display, discard);

  {
    lv::mcp::server<"obj", "label", "button", "arc", "bar", "slider", "switch", "checkbox"> agent{
        lv::screen_active(),
        {.name = "lvgl26 example host", .screenshot_directory = argc > 2 ? argv[2] : ""}};
    agent.serve(in, stdout);
  }

  lv_deinit();
  if (in != stdin) std::fclose(in);
  return 0;
}
