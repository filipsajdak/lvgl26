#pragma once

// A Model Context Protocol server inside the program that shows the UI, so that
// an agent can change the screen and check the result. Hosted: it allocates and
// uses <cstdio>.
//
//   lv::mcp::server<"obj", "label", "arc"> agent{lv::screen_active()};
//   agent.serve();   // answers requests on stdin until it is closed
//
// The widgets in <> are the ones a document may use: see lv/runtime.hpp.
//
// Tools:
//   load_document  builds a screen from a runtime document under the widget the
//                  server was given, in place of the document loaded before
//   apply_patch    changes properties of that screen by id
//   dump_tree      the widget tree under the server's widget as JSON (lv/dump.hpp)
//   screenshot     that widget as a PNG: written to "path", or returned as an image
//
// handle() takes one message and returns the answer, or an empty text when the
// message was a notification. It never fails on its input: anything that is not
// a request gets a JSON-RPC error. serve() is handle() for every line of a stream.
//
// Protocol: JSON-RPC 2.0, one message per line. Two revisions are answered:
//   2026-07-28            no handshake. Every request names the revision and the
//                         client's capabilities in params._meta; server/discover
//                         tells what the server can do.
//   2025-11-25 and older  a request without that _meta is taken as one of a client
//                         that starts with initialize.
//
// Limits: keys and tool names are compared as written, so a client that escapes
// them ("tools\/call") is not understood. A "path" with a backslash is refused.
// serve() blocks: nothing else of the program runs while it waits for a line.
// Keep LVGL's log off stdout (LV_LOG_PRINTF), which belongs to the protocol.

#include <lv/detail/enum_names.hpp>
#include <lv/json.hpp>
#include <lv/json_writer.hpp>
#include <lv/png.hpp>
#include <lv/runtime.hpp>

#include <array>
#include <cstdint>
#include <cstdio>
#include <expected>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace lv::mcp {

// How the server names itself, and what it may do. The texts must outlive the server.
struct options {
  std::string_view name = "lvgl26";
  std::string_view version = "0.0.1";
  // Whether screenshot may write a file where the client says.
  bool write_files = true;
};

// The revision whose requests carry their own metadata.
inline constexpr std::string_view protocol_version = "2026-07-28";

// The revisions that start with initialize, newest first.
inline constexpr std::array<std::string_view, 4> handshake_versions{
    "2025-11-25", "2025-06-18", "2025-03-26", "2024-11-05"};

namespace detail {

inline constexpr int parse_error = -32700;
inline constexpr int invalid_request = -32600;
inline constexpr int method_not_found = -32601;
inline constexpr int invalid_params = -32602;
inline constexpr int unsupported_protocol_version = -32022;

inline constexpr std::string_view version_key = "io.modelcontextprotocol/protocolVersion";
inline constexpr std::string_view capabilities_key = "io.modelcontextprotocol/clientCapabilities";

inline constexpr std::string_view instructions =
    "Tools to see and change an LVGL screen. load_document builds a screen, apply_patch "
    "changes it by id, dump_tree and screenshot show the result.";

// A sink that appends to a string.
struct appender {
  std::string& to;
  void operator()(std::string_view piece) const { to += piece; }
};

// `text` as a JSON string, appended to `to`.
inline void append_string(std::string& to, std::string_view text) {
  appender out{to};
  json::write_string(out, text);
}

constexpr bool has_control_character(std::string_view text) {
  for (const char c : text)
    if (static_cast<unsigned char>(c) < 0x20) return true;
  return false;
}

// The name of an enumerator, for a message.
template <class E>
std::string_view name_of(E value) {
  for (const auto& e : lv::detail::enum_names<E>)
    if (e.value == static_cast<std::int64_t>(value)) return e.name;
  return "error";
}

// ---- messages ------------------------------------------------------------------

// An error response. `id` is the id as the client wrote it, or empty when it could
// not be read. `data` is JSON, or empty.
inline std::string error_message(std::string_view id, int code, std::string_view message,
                                 std::string_view data = {}) {
  std::string out = R"({"jsonrpc":"2.0","id":)";
  out += id.empty() ? std::string_view{"null"} : id;
  out += R"(,"error":{"code":)";
  out += std::to_string(code);
  out += R"(,"message":)";
  append_string(out, message);
  if (!data.empty()) out += R"(,"data":)";
  out += data;
  out += "}}";
  return out;
}

inline std::string server_info(const options& server) {
  std::string out = R"({"name":)";
  append_string(out, server.name);
  out += R"(,"version":)";
  append_string(out, server.version);
  out += "}";
  return out;
}

// What a result of the 2026-07-28 revision starts with.
inline std::string result_head(const options& server) {
  return R"("resultType":"complete","_meta":{"io.modelcontextprotocol/serverInfo":)" +
         server_info(server) + "}";
}

// A result response. `body` is the members of the result, without braces.
inline std::string result_message(std::string_view id, bool modern, const options& server,
                                  std::string_view body) {
  std::string out = R"({"jsonrpc":"2.0","id":)";
  out += id;
  out += R"(,"result":{)";
  if (modern) out += result_head(server);
  if (modern && !body.empty()) out += ",";
  out += body;
  out += "}}";
  return out;
}

// A request or a notification, as views of the message.
struct request {
  std::string_view id;  // as written, quotes included; empty for a notification
  std::string_view method;
  json::value params;
};

// An id is a number, or a string that can be sent back on one line.
inline bool usable_id(json::value id) {
  if (id.is(json::kind::number)) return true;
  return id.is(json::kind::string) && !has_control_character(id.text());
}

// Reads a message. The error is the response to send.
inline std::expected<request, std::string> read_request(std::string_view message) {
  const auto parsed = json::value::parse(message);
  if (!parsed) return std::unexpected(error_message({}, parse_error, "Parse error: not JSON"));
  const auto id = (*parsed)["id"];
  if (id && !usable_id(id))
    return std::unexpected(
        error_message({}, invalid_request, "Invalid Request: the id must be a string or a number"));
  const auto method = (*parsed)["method"];
  if ((*parsed)["jsonrpc"].text() != R"("2.0")" || !method.is(json::kind::string))
    return std::unexpected(error_message(
        id.text(), invalid_request,
        R"(Invalid Request: a message is an object with "jsonrpc": "2.0" and a "method")"));
  return request{id.text(), method.string(), (*parsed)["params"]};
}

inline std::string unsupported_version(std::string_view id, std::string_view requested) {
  std::string data = R"({"supported":[")";
  data += protocol_version;
  data += R"("],"requested":)";
  append_string(data, requested);
  data += "}";
  return error_message(id, unsupported_protocol_version, "Unsupported protocol version", data);
}

// Whether a request is one of the 2026-07-28 revision: it names that revision in
// its _meta. The error is the response to a request that names another revision,
// or leaves out the capabilities that revision requires.
inline std::expected<bool, std::string> is_modern(const request& r) {
  const auto meta = r.params["_meta"];
  const auto version = meta[version_key];
  if (!version) return false;
  if (version.string() != protocol_version)
    return std::unexpected(unsupported_version(r.id, version.string()));
  if (!meta[capabilities_key].is(json::kind::object))
    return std::unexpected(error_message(
        r.id, invalid_params, "Invalid params: _meta needs io.modelcontextprotocol/clientCapabilities"));
  return true;
}

// The result of initialize: the revision the client asked for if it is one of
// ours, the newest one otherwise.
inline std::string handshake(json::value params, const options& server) {
  std::string_view version = handshake_versions.front();
  for (const auto known : handshake_versions)
    if (params["protocolVersion"].string() == known) version = known;
  std::string out = R"("protocolVersion":")";
  out += version;
  out += R"(","capabilities":{"tools":{}},"serverInfo":)";
  out += server_info(server);
  out += R"(,"instructions":)";
  append_string(out, instructions);
  return out;
}

// The result of server/discover.
inline std::string discovery() {
  std::string out = R"("supportedVersions":[")";
  out += protocol_version;
  out += R"("],"capabilities":{"tools":{}},"instructions":)";
  append_string(out, instructions);
  return out;
}

// ---- tools ---------------------------------------------------------------------

// The result of tools/list. `widgets` names the widgets a document may use.
inline std::string tool_list(std::string_view widgets) {
  std::string out =
      R"("tools":[{"name":"load_document","description":"Builds a screen from a document, )"
      R"(in place of the one loaded before. A node is {\"type\": widget, \"id\": text, )"
      R"(\"props\": {name: value}, \"children\": [nodes]}. Values: numbers, true or false, text, )"
      R"(\"120px\", \"50%\", \"content\", \"90deg\", \"300ms\", \"#rrggbb\", an enumerator by name )"
      R"((\"center\"), a list for a property that takes several values. \"style\" in props takes )"
      R"(style properties. Nothing changes if the document is refused. Widgets: )";
  out += widgets;
  out +=
      R"(.","inputSchema":{"type":"object","properties":{"document":{"type":"object"}},)"
      R"("required":["document"]}},)"
      R"({"name":"apply_patch","description":"Changes properties of the loaded screen by id: )"
      R"({\"speedo\": {\"value\": 60}}. Values are written as in a document.",)"
      R"("inputSchema":{"type":"object","properties":{"patch":{"type":"object"}},)"
      R"("required":["patch"]}},)"
      R"({"name":"dump_tree","description":"The widget tree as JSON. Per widget: type, id, )"
      R"(x, y, width and height in pixels, the properties that can be read, children.",)"
      R"("inputSchema":{"type":"object","additionalProperties":false}},)"
      R"({"name":"screenshot","description":"The screen as a PNG picture. With \"path\" the file )"
      R"(is written there; without it the picture is returned.",)"
      R"("inputSchema":{"type":"object","properties":{"path":{"type":"string"}}}}])";
  return out;
}

// The result of a tool that answers with a text.
inline std::string text_result(std::string_view text, bool failed = false) {
  std::string out = R"("content":[{"type":"text","text":)";
  append_string(out, text);
  out += R"(}],"isError":)";
  out += failed ? "true" : "false";
  return out;
}

inline void append_base64(std::string& out, std::span<const std::uint8_t> bytes) {
  constexpr std::string_view digits =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  for (std::size_t i = 0; i < bytes.size(); i += 3) {
    const std::size_t left = bytes.size() - i;
    std::uint32_t group = std::uint32_t{bytes[i]} << 16;
    if (left > 1) group |= std::uint32_t{bytes[i + 1]} << 8;
    if (left > 2) group |= bytes[i + 2];
    out += digits[group >> 18];
    out += digits[(group >> 12) & 63];
    out += left > 1 ? digits[(group >> 6) & 63] : '=';
    out += left > 2 ? digits[group & 63] : '=';
  }
}

// The result of a tool that answers with a PNG picture.
inline std::string image_result(std::span<const std::uint8_t> png) {
  std::string out = R"("content":[{"type":"image","data":")";
  append_base64(out, png);
  out += R"(","mimeType":"image/png"}],"isError":false)";
  return out;
}

// A refused document or patch, as a result the agent can act on.
inline std::string failure(const load_failure& f) {
  std::string text{name_of(f.error)};
  text += ": ";
  text += f.at.substr(0, 200);
  return text_result(text, true);
}

inline std::string failure(png_error error) { return text_result(name_of(error), true); }

// A file name that needs no unescaping, which the JSON reader does not do.
inline bool usable_path(json::value path) {
  return path.is(json::kind::string) && !path.string().empty() &&
         path.string().find('\\') == std::string_view::npos && !has_control_character(path.text());
}

// ---- the stream ----------------------------------------------------------------

// Reads up to the next line feed. False at the end of the stream.
inline bool read_line(std::FILE* in, std::string& line) {
  line.clear();
  int c = std::fgetc(in);
  for (; c != EOF && c != '\n'; c = std::fgetc(in)) line.push_back(static_cast<char>(c));
  return c != EOF || !line.empty();
}

inline void write_line(std::FILE* out, const std::string& line) {
  std::fputs(line.c_str(), out);
  std::fputc('\n', out);
  std::fflush(out);
}

}  // namespace detail

// See the top of this file.
template <lv::detail::fixed... Widgets>
class server {
  using ui = runtime<Widgets...>;

  obj parent_;
  options options_;
  typename ui::loaded screen_;

 public:
  // The server shows documents under `parent`, and dumps and draws `parent`.
  explicit server(obj parent, options o = {})
    pre(parent.raw() != nullptr)
      : parent_(parent), options_(o) {}

  // The answer to one message, or an empty text if it needs none.
  std::string handle(std::string_view message) {
    const auto request = detail::read_request(message);
    if (!request) return request.error();
    if (request->id.empty()) return {};  // a notification is never answered
    const auto modern = detail::is_modern(*request);
    if (!modern) return modern.error();
    return answer(*request, *modern);
  }

  // Answers every line of `in` on `out`, until `in` ends.
  void serve(std::FILE* in = stdin, std::FILE* out = stdout)
    pre(in != nullptr)
    pre(out != nullptr)
  {
    std::string line;
    while (detail::read_line(in, line)) {
      if (line.find_first_not_of(" \t\r") == std::string::npos) continue;
      const std::string reply = handle(line);
      if (!reply.empty()) detail::write_line(out, reply);
    }
  }

 private:
  std::string answer(const detail::request& r, bool modern) {
    const auto result = [&](std::string_view body) {
      return detail::result_message(r.id, modern, options_, body);
    };
    if (r.method == "ping") return result({});
    if (r.method == "tools/list") return result(detail::tool_list(widget_names()));
    if (r.method == "tools/call") return call(r, modern);
    if (r.method == "server/discover") return discover(r, modern);
    if (r.method == "initialize" && !modern) return result(detail::handshake(r.params, options_));
    return detail::error_message(r.id, detail::method_not_found, "Method not found");
  }

  // server/discover belongs to the revision whose requests name it in _meta.
  std::string discover(const detail::request& r, bool modern) const {
    if (!modern)
      return detail::error_message(
          r.id, detail::invalid_params,
          "Invalid params: server/discover needs _meta with io.modelcontextprotocol/protocolVersion");
    return detail::result_message(r.id, modern, options_, detail::discovery());
  }

  std::string call(const detail::request& r, bool modern) {
    const auto name = r.params["name"];
    const auto arguments = r.params["arguments"];
    if (!name.is(json::kind::string) || (arguments && !arguments.is(json::kind::object)))
      return detail::error_message(
          r.id, detail::invalid_params,
          "Invalid params: tools/call takes the name of a tool and an object of arguments");
    const auto body = run(name.string(), arguments);
    if (!body) {
      std::string text = "Unknown tool: ";
      text += name.string();
      return detail::error_message(r.id, detail::invalid_params, text);
    }
    return detail::result_message(r.id, modern, options_, *body);
  }

  // The result of the tool called `name`, or nothing if there is no such tool.
  std::optional<std::string> run(std::string_view name, json::value arguments) {
    if (name == "load_document") return load(arguments["document"]);
    if (name == "apply_patch") return patch(arguments["patch"]);
    if (name == "dump_tree") return dump();
    if (name == "screenshot") return screenshot(arguments["path"]);
    return std::nullopt;
  }

  std::string load(json::value document) {
    if (!document.is(json::kind::object))
      return detail::text_result(R"(load_document takes "document": an object)", true);
    auto loaded = ui::load(document.text(), parent_);
    if (!loaded) return detail::failure(loaded.error());
    screen_ = std::move(*loaded);  // the screen loaded before is deleted here
    return detail::text_result("loaded");
  }

  std::string patch(json::value changes) const {
    if (!changes) return detail::text_result(R"(apply_patch takes "patch": an object)", true);
    const auto applied = screen_.apply(changes.text());
    if (!applied) return detail::failure(applied.error());
    return detail::text_result("applied");
  }

  std::string dump() const {
    std::string text;
    detail::appender out{text};
    ui::dump(parent_, out, &screen_);
    return detail::text_result(text);
  }

  std::string screenshot(json::value path) const {
    if (path) return screenshot_file(path);
    const auto png = encode_png(parent_);
    if (!png) return detail::failure(png.error());
    return detail::image_result(*png);
  }

  std::string screenshot_file(json::value path) const {
    if (!detail::usable_path(path))
      return detail::text_result(R"("path" must be a file name without backslashes)", true);
    if (!options_.write_files)
      return detail::text_result(
          "this server does not write files: call screenshot without a path", true);
    const std::string file{path.string()};
    const auto written = write_png(parent_, file.c_str());
    if (!written) return detail::failure(written.error());
    return detail::text_result("wrote " + file);
  }

  static std::string widget_names() {
    std::string out = "column, row";
    ((out += ", ", out += Widgets.sv()), ...);
    return out;
  }
};

}  // namespace lv::mcp
