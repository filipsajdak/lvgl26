#pragma once

// Editor stub of lv/mcp.hpp: a Model Context Protocol server inside the application.

#include <lv/runtime.hpp>
#include <lv/view.hpp>

#include <array>
#include <cstdio>
#include <string>
#include <string_view>

namespace lv::mcp {

/// How the server names itself, and what it may do. The texts must outlive the server.
struct options {
  std::string_view name = "lvgl26";
  std::string_view version = "0.0.1";
  /// The directory screenshot may make files in. Empty: it writes no files.
  std::string_view screenshot_directory = {};
};

/// The revision whose requests carry their own metadata.
inline constexpr std::string_view protocol_version = "2026-07-28";

/// The older revisions, which start with a handshake.
inline constexpr std::array<std::string_view, 4> handshake_versions{"2025-11-25", "2025-06-18", "2025-03-26",
                                                                    "2024-11-05"};

/// Lets an agent drive a screen: load_document, apply_patch, dump_tree, screenshot.
/// It may create the widgets it is given: lv::mcp::server<"obj", "label", "button">.
template <lv::detail::fixed... Widgets>
  requires(lv::detail::known_widget<Widgets> && ...)
class server {
 public:
  /// The screens it loads go under `parent`.
  explicit server(obj parent, options o = {});

  /// Answers one JSON-RPC message. Empty for a notification.
  std::string handle(std::string_view message);

  /// Answers the messages of `in`, one for each line, until it ends.
  void serve(std::FILE* in = stdin, std::FILE* out = stdout);
};

}  // namespace lv::mcp
