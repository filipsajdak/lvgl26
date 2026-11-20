// The agent kit: the tree dump, the PNG of a screen, and the MCP server that
// lets an agent load a screen, change it and check the result.

#include <lv/lv.hpp>
#include <lv/mcp.hpp>
#include <lv/runtime.hpp>

#include "check.hpp"

#include <csignal>
#include <cstring>
#include <filesystem>
#include <string>
#include <string_view>

#include <sys/resource.h>

namespace {

using ui = lv::runtime<"obj", "label", "arc", "button">;
using server = lv::mcp::server<"obj", "label", "arc", "button">;

bool has(std::string_view text, std::string_view part) {
  return text.find(part) != std::string_view::npos;
}

// A reply is one line of JSON.
bool one_json_line(std::string_view reply) {
  return !has(reply, "\n") && lv::json::value::parse(reply).has_value();
}

std::string read_file(const std::string& path) {
  std::string bytes;
  if (std::FILE* f = std::fopen(path.c_str(), "rb")) {
    for (int c = std::fgetc(f); c != EOF; c = std::fgetc(f)) bytes.push_back(static_cast<char>(c));
    std::fclose(f);
  }
  return bytes;
}

constexpr std::string_view png_signature = "\x89PNG\r\n\x1a\n";

// The pixels LVGL draws for a widget.
struct picture {
  lv_draw_buf_t* buf;
  explicit picture(lv_obj_t* of) : buf(lv_snapshot_take(of, LV_COLOR_FORMAT_ARGB8888)) {}
  picture(const picture&) = delete;
  picture& operator=(const picture&) = delete;
  ~picture() {
    if (buf) lv_draw_buf_destroy(buf);
  }
  bool same_as(const picture& other) const {
    if (!buf || !other.buf || buf->header.w != other.buf->header.w ||
        buf->header.h != other.buf->header.h)
      return false;
    const auto size = static_cast<std::size_t>(buf->header.stride) * buf->header.h;
    return std::memcmp(buf->data, other.buf->data, size) == 0;
  }
};

// Whether the PNG file at `path`, read by LVGL's own decoder, has the pixels of `of`.
bool file_shows(const std::string& path, lv::obj of) {
  const picture expected{of.raw()};
  lv_obj_t* image = lv_image_create(lv_layer_top());
  lv_image_set_src(image, ("A:" + path).c_str());
  const picture decoded{image};
  lv_obj_delete(image);
  return decoded.same_as(expected);
}

void json_text() {
  std::string text;
  auto sink = [&](std::string_view piece) { text += piece; };
  lv::json::write_string(sink, std::string{"a\"b\\c\n\r\t\x01z"});
  check(text == R"("a\"b\\c\n\r\t\u0001z")");
  text.clear();
  lv::json::write_string(sink, std::string{"\x1f \x7f"});
  check(text == "\"\\u001f \x7f\"");
  text.clear();
  lv::json::write_number(sink, std::stoll("-9223372036854775807"));
  check(text == "-9223372036854775807");
}

// The writer passes each well-formed UTF-8 character as it is and writes U+FFFD for
// each byte that is no part of one: the bounds of every range of table 3-7, from both sides.
void the_writer_writes_only_well_formed_utf8() {
  const auto written = [](std::string_view bytes) {
    std::string text;
    auto sink = [&](std::string_view piece) { text += piece; };
    lv::json::write_string(sink, std::string{bytes});
    return text.substr(1, text.size() - 2);
  };
  const auto replaced = [](std::size_t bytes) {
    std::string text;
    for (std::size_t i = 0; i < bytes; ++i) text += "\xef\xbf\xbd";
    return text;
  };
  for (const char* character :
       {"\x7f", "\xc2\x80", "\xdf\xbf", "\xe0\xa0\x80", "\xe0\xbf\xbf", "\xe1\x80\x80", "\xec\xbf\xbf",
        "\xed\x80\x80", "\xed\x9f\xbf", "\xee\x80\x80", "\xef\xbf\xbf", "\xf0\x90\x80\x80", "\xf0\xbf\xbf\xbf",
        "\xf1\x80\x80\x80", "\xf3\xbf\xbf\xbf", "\xf4\x80\x80\x80", "\xf4\x8f\xbf\xbf", "\xf0\x9f\x98\x80"}) {
    check(written(character) == character);
    check(lv::json::detail::well_formed(std::string{character}));
  }
  for (const char* broken :
       {"\x80", "\xbf", "\xc0\x80", "\xc1\xbf", "\xe0\x9f\xbf", "\xe0\xc0\x80", "\xed\xa0\x80", "\xed\xbf\xbf",
        "\xf0\x8f\xbf\xbf", "\xf0\xc0\x80\x80", "\xf4\x90\x80\x80", "\xf5\x80\x80\x80", "\xff",
        "\xc2\xc0", "\xe1\x80\xc0", "\xf1\x80\x80\xc0", "\xf1\x80\xc0\x80"}) {
    check(written(broken) == replaced(std::strlen(broken)));
    check(!lv::json::detail::well_formed(std::string{broken}));
  }
  // A character cut short at the end, and one cut by an ASCII byte.
  check(written(std::string{"a\xc3"}) == "a" + replaced(1));
  check(written(std::string{"\xe2\x82"}) == replaced(2));
  check(written(std::string{"\xf0\x9f\x98"}) == replaced(3));
  check(written(std::string{"\xc2\x7f"}) == replaced(1) + "\x7f");
  check(written(std::string{"\xe1\x7f\x80"}) == replaced(1) + "\x7f" + replaced(1));
  check(written(std::string{"\xf4\x7f\x80\x80"}) == replaced(1) + "\x7f" + replaced(2));
  check(written(std::string{"\xf1\x80\x80\"z"}) == replaced(3) + "\\\"z");
  check(!lv::json::detail::well_formed(std::string{"ok\xe2\x82"}));
  check(lv::json::detail::well_formed(std::string{}) && lv::json::detail::well_formed(std::string{"ok\xe2\x82\xac"}));

  // A cut steps back to the start of the character it would split.
  using lv::json::detail::utf8_prefix;
  const std::string euro = "ab\xe2\x82\xac";
  check(utf8_prefix(euro, 2) == "ab" && utf8_prefix(euro, 3) == "ab" && utf8_prefix(euro, 4) == "ab");
  check(utf8_prefix(euro, 5) == euro && utf8_prefix(euro, 9) == euro && utf8_prefix(euro, 0).empty());
  const std::string emoji = "a\xf0\x9f\x98\x80z";
  check(utf8_prefix(emoji, 4) == "a" && utf8_prefix(emoji, 5) == "a\xf0\x9f\x98\x80");
  // No further than one character reaches, whatever the bytes are.
  const std::string continuations(8, '\x80');
  check(utf8_prefix(continuations, 4).size() == 1 && utf8_prefix(continuations, 3).empty());
}

void tree_dump(lv::obj screen) {
  auto loaded = ui::load(R"({"type": "column", "id": "root", "props": {"size": [200, 100]}, "children": [
      {"type": "label", "id": "title", "props": {"text": "say \"hi\""}},
      {"type": "arc", "id": "speedo", "props": {"range": [0, 240], "value": 120, "size": [50, 60]}},
      {"type": "button"}]})", screen);
  check(loaded.has_value());
  if (!loaded) return;

  std::string text;
  loaded->dump([&](std::string_view piece) { text += piece; });
  check(lv::json::value::parse(text).has_value());
  const lv::json::value root{text};
  check(root["type"].string() == "obj" && root["id"].string() == "root");
  check(root["x"].number() == 0 && root["y"].number() == 0);
  check(root["width"].number() == 200 && root["height"].number() == 100);
  check(root["children"].size() == 3);

  const auto title = root["children"].at(0);
  check(title["type"].string() == "label" && title["id"].string() == "title");
  // The label holds the text as the document wrote it; the dump escapes it again.
  check(title["props"]["text"].text() == R"("say \\\"hi\\\"")");
  check(title["props"]["recolor"].text() == "false");
  check(title["props"]["long_mode"].string() == "LV_LABEL_LONG_MODE_WRAP");

  const auto speedo = root["children"].at(1);
  check(speedo["type"].string() == "arc" && speedo["id"].string() == "speedo");
  check(speedo["props"]["value"].number() == 120 && speedo["props"]["max_value"].number() == 240);
  check(speedo["width"].number() == 50 && speedo["height"].number() == 60);
  check(speedo["x"].number() > 0 && speedo["y"].number() > title["y"].number());
  // What obj has, every widget has.
  check(speedo["props"]["width"].number() == 50);
  check(speedo["props"]["state"].string() == "LV_STATE_DEFAULT");

  const auto button = root["children"].at(2);
  check(button["type"].string() == "button" && !button["id"] && !button["children"]);

  // A state made of two flags is not one enumerator: it is written as a number.
  lv_obj_add_state(loaded->find("speedo").raw(),
                   static_cast<lv_state_t>(LV_STATE_CHECKED | LV_STATE_FOCUSED));
  text.clear();
  loaded->dump([&](std::string_view piece) { text += piece; });
  check(lv::json::value{text}["children"].at(1)["props"]["state"].number() ==
        (LV_STATE_CHECKED | LV_STATE_FOCUSED));

  // From the screen down, without ids; a widget of a kind that is not listed has
  // no type and the properties of obj.
  using few = lv::runtime<"label">;
  text.clear();
  few::dump(screen, [&](std::string_view piece) { text += piece; });
  check(lv::json::value::parse(text).has_value());
  const auto first = lv::json::value{text}["children"].at(0);
  check(lv::json::value{text}["type"].text() == "null" && !first["id"]);
  check(first["children"].at(0)["type"].string() == "label");
  const auto arc = first["children"].at(1);
  check(arc["type"].text() == "null" && !arc["props"]["value"] && arc["props"]["width"].number() == 50);

  // With the ids of a screen that is given.
  text.clear();
  ui::dump(screen, [&](std::string_view piece) { text += piece; }, &*loaded);
  check(lv::json::value{text}["children"].at(0)["id"].string() == "root");
  check(loaded->id_of(loaded->find("speedo")) == "speedo");
  check(loaded->id_of(screen).empty() && loaded->id_of(lv::obj{}).empty());

  // A deleted screen is null.
  lv_obj_delete(loaded->root().raw());
  check(loaded->id_of(lv::obj{}).empty());
  text.clear();
  loaded->dump([&](std::string_view piece) { text += piece; });
  check(text == "null");
}

// A color and a text that is missing, which no getter of the widgets above returns.
void values() {
  std::string text;
  auto sink = [&](std::string_view piece) { text += piece; };
  const auto out = lv::detail::erase_sink(sink);
  lv::detail::write_value(out, lv_color_hex(static_cast<std::uint32_t>(std::stoul("1193722"))));
  check(text == R"("#1236fa")");
  text.clear();
  const char* missing = std::strlen(text.c_str()) == 0 ? nullptr : "x";
  lv::detail::write_value(out, missing);
  check(text == "null");
}

void pictures(lv::obj screen, const std::string& dir) {
  auto loaded = ui::load(R"({"type": "column", "props": {"size": ["100%", "100%"]}, "children": [
      {"type": "label", "props": {"text": "Speed"}},
      {"type": "arc", "id": "speedo", "props": {"value": 40}}]})", screen);
  check(loaded.has_value());

  // LVGL's own decoder reads the file back to the pixels of the screen.
  const std::string path = dir + "/agent_kit_screen.png";
  std::remove(path.c_str());
  check(lv::write_png(screen, path.c_str()).has_value());
  check(read_file(path).starts_with(png_signature));
  check(file_shows(path, screen));
  // write_png replaces a file. write_new_png never does, and does not follow a link.
  check(lv::write_png(screen, path.c_str()).has_value());
  const auto again = lv::write_new_png(screen, path.c_str());
  check(!again && again.error() == lv::png_error::file_exists);
  const std::string fresh = dir + "/agent_kit_new.png";
  const std::string link = dir + "/agent_kit_link.png";
  const std::string target = dir + "/agent_kit_link_target.png";
  for (const auto& old : {fresh, link, target}) std::filesystem::remove(old);
  check(lv::write_new_png(screen, fresh.c_str()).has_value() && file_shows(fresh, screen));
  std::error_code no_link;
  std::filesystem::create_symlink(target, link, no_link);
  check(!no_link);
  const auto through = lv::write_new_png(screen, link.c_str());
  check(!through && through.error() == lv::png_error::file_exists);
  check(!std::filesystem::exists(target));
  const auto missing = lv::write_new_png(screen, (dir + "/no/such/dir/x.png").c_str());
  check(!missing && missing.error() == lv::png_error::cannot_write);
  // Flat areas make it far smaller than its 320 * 240 * 4 bytes.
  check(read_file(path).size() < 40000);

  // One widget, with its own size.
  const auto arc = lv::encode_png(loaded->find("speedo"));
  check(arc.has_value());
  if (arc) {
    const std::string_view bytes{reinterpret_cast<const char*>(arc->data()), arc->size()};
    check(bytes.starts_with(png_signature) && bytes.substr(12, 4) == "IHDR");
    // As wide as the arc and what it draws around itself: less than the screen.
    check((*arc)[18] == 0 && (*arc)[19] >= lv_obj_get_width(loaded->find("speedo").raw()));
    check((*arc)[19] < 200);
    check(bytes.ends_with(std::string_view{"IEND\xae\x42\x60\x82"}));
  }

  // A file that cannot be written; a widget with nothing to draw.
  const auto nowhere = lv::write_png(screen, (dir + "/no/such/dir/x.png").c_str());
  check(!nowhere && nowhere.error() == lv::png_error::cannot_write);
  auto empty = screen.add<"obj">();
  lv_obj_set_size(empty.raw(), 0, 0);
  const auto nothing = lv::write_png(empty, path.c_str());
  check(!nothing && nothing.error() == lv::png_error::no_picture);
  lv_obj_delete(empty.raw());
}

// Runs of every length the encoder treats differently, read back by LVGL.
void picture_runs(lv::obj screen, const std::string& dir) {
  lv_obj_set_style_bg_color(screen.raw(), lv_color_hex(0x102030), 0);
  int x = 0;
  for (const int width : {1, 2, 3, 4, 63, 64, 65, 66, 1}) {
    auto box = screen.add<"obj">();
    lv_obj_remove_style_all(box.raw());
    lv_obj_set_style_bg_opa(box.raw(), LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(box.raw(), lv_color_hex(0xF09000U + static_cast<std::uint32_t>(x)), 0);
    lv_obj_set_pos(box.raw(), x, width);
    lv_obj_set_size(box.raw(), width, 3);
    x += width + (width % 3) + 1;
  }
  const std::string path = dir + "/agent_kit_runs.png";
  std::remove(path.c_str());
  check(lv::write_png(screen, path.c_str()).has_value());
  check(file_shows(path, screen));
  lv_obj_clean(screen.raw());
}

std::string call(std::string_view id, std::string_view tool, std::string_view arguments) {
  std::string m = R"({"jsonrpc": "2.0", "id": )";
  m += id;
  m += R"(, "method": "tools/call", "params": {"name": ")";
  m += tool;
  m += R"(", "arguments": )";
  m += arguments;
  m += "}}";
  return m;
}

// A client that starts with initialize: load, patch, dump, screenshot.
void agent_changes_the_screen(lv::obj screen, const std::string& dir) {
  server agent{screen, {.name = "test \"host\"", .version = "1.2", .screenshot_directory = dir}};

  const auto hello = agent.handle(R"({"jsonrpc": "2.0", "id": 1, "method": "initialize", "params":
      {"protocolVersion": "2025-06-18", "capabilities": {}, "clientInfo": {"name": "t", "version": "1"}}})");
  check(one_json_line(hello));
  const auto hello_result = lv::json::value{hello}["result"];
  check(lv::json::value{hello}["id"].text() == "1");
  check(hello_result["protocolVersion"].string() == "2025-06-18");
  check(hello_result["capabilities"]["tools"].is(lv::json::kind::object));
  check(hello_result["serverInfo"]["name"].text() == R"("test \"host\"")");
  check(hello_result["serverInfo"]["version"].string() == "1.2");
  check(!hello_result["resultType"] && hello_result["instructions"].is(lv::json::kind::string));
  // A revision we do not know is answered with the newest one that has a handshake.
  check(has(agent.handle(R"({"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"1999-01-01"}})"),
            R"("protocolVersion":"2025-11-25")"));
  check(has(agent.handle(R"({"jsonrpc":"2.0","id":1,"method":"initialize"})"),
            R"("protocolVersion":"2025-11-25")"));
  check(has(agent.handle(R"({"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2024-11-05"}})"),
            R"("protocolVersion":"2024-11-05")"));

  // Notifications are never answered, known or not.
  check(agent.handle(R"({"jsonrpc": "2.0", "method": "notifications/initialized"})").empty());
  check(agent.handle(R"({"jsonrpc": "2.0", "method": "notifications/whatever", "params": [1]})").empty());
  check(agent.handle(R"({"jsonrpc":"2.0","id":"p","method":"ping"})") ==
        R"({"jsonrpc":"2.0","id":"p","result":{}})");

  const auto list = agent.handle(R"({"jsonrpc": "2.0", "id": "list-1", "method": "tools/list"})");
  check(one_json_line(list) && lv::json::value{list}["id"].string() == "list-1");
  const auto tools = lv::json::value{list}["result"]["tools"];
  check(tools.size() == 4);
  // The hints for a cache belong to the 2026-07-28 revision only.
  check(!lv::json::value{list}["result"]["ttlMs"] && !lv::json::value{list}["result"]["cacheScope"]);
  check(tools.at(0)["name"].string() == "load_document" && tools.at(1)["name"].string() == "apply_patch");
  check(tools.at(2)["name"].string() == "dump_tree" && tools.at(3)["name"].string() == "screenshot");
  for (std::size_t i = 0; i < tools.size(); ++i) {
    check(tools.at(i)["description"].is(lv::json::kind::string));
    check(tools.at(i)["inputSchema"]["type"].string() == "object");
  }
  check(has(tools.at(0)["description"].string(), "Widgets: column, row, obj, label, arc, button."));

  // Before a document is loaded there is nothing to patch.
  const auto early = agent.handle(call("2", "apply_patch", R"({"patch": {"speedo": {"value": 1}}})"));
  check(has(early, R"("isError":true)") && has(early, "unknown_id: speedo"));

  const auto before = lv_obj_get_child_count(screen.raw());
  const auto loaded = agent.handle(call("3", "load_document", R"({"document": {"type": "column", "children": [
      {"type": "label", "id": "title", "props": {"text": "Speed"}},
      {"type": "arc", "id": "speedo", "props": {"range": [0, 240], "value": 120}}]}})"));
  check(one_json_line(loaded) && has(loaded, R"("text":"loaded")") && has(loaded, R"("isError":false)"));
  check(lv_obj_get_child_count(screen.raw()) == before + 1);

  const auto dump = agent.handle(call("4", "dump_tree", "{}"));
  check(one_json_line(dump) && has(dump, R"(\"id\":\"speedo\")") && has(dump, R"(\"value\":120)"));
  check(has(dump, R"(\"text\":\"Speed\")") && has(dump, R"("isError":false)"));

  const auto patched = agent.handle(call("5", "apply_patch",
                                         R"({"patch": {"speedo": {"value": 60}, "title": {"text": "Trip"}}})"));
  check(has(patched, R"("text":"applied")") && has(patched, R"("isError":false)"));
  const auto after = agent.handle(call("6", "dump_tree", "{}"));
  check(has(after, R"(\"value\":60)") && !has(after, R"(\"value\":120)"));
  check(has(after, R"(\"text\":\"Trip\")"));
  // Arguments may be left out when a tool takes none.
  check(has(agent.handle(R"({"jsonrpc":"2.0","id":7,"method":"tools/call","params":{"name":"dump_tree"}})"),
            R"(\"value\":60)"));

  // A screenshot to a new file in the directory the host allows: a PNG of what
  // is on the screen now.
  const std::string path = dir + "/agent_kit_mcp.png";
  std::remove(path.c_str());
  const auto shot = agent.handle(call("8", "screenshot", R"({"name": "agent_kit_mcp.png"})"));
  check(one_json_line(shot) && has(shot, "wrote " + path) && has(shot, R"("isError":false)"));
  check(read_file(path).starts_with(png_signature));
  check(file_shows(path, screen));
  // A file that exists is not replaced, and a link is not followed.
  const std::string kept = read_file(path);
  const auto twice = agent.handle(call("8", "screenshot", R"({"name": "agent_kit_mcp.png"})"));
  check(has(twice, R"("isError":true)") && has(twice, "file_exists") && read_file(path) == kept);
  const auto linked = agent.handle(call("8", "screenshot", R"({"name": "agent_kit_link.png"})"));
  check(has(linked, R"("isError":true)") && has(linked, "file_exists"));
  check(!std::filesystem::exists(dir + "/agent_kit_link_target.png"));

  // A screenshot as an image: the same file, in base64.
  const auto image = agent.handle(call("9", "screenshot", "{}"));
  check(one_json_line(image));
  const auto content = lv::json::value{image}["result"]["content"].at(0);
  check(content["type"].string() == "image" && content["mimeType"].string() == "image/png");
  std::string encoded;
  const std::string file = read_file(path);
  lv::mcp::detail::append_base64(
      encoded, {reinterpret_cast<const std::uint8_t*>(file.data()), file.size()});
  check(content["data"].string() == encoded && content["data"].string().starts_with("iVBORw0KGgo"));

  // A second document takes the place of the first; a refused one changes nothing.
  check(has(agent.handle(call("10", "load_document", R"({"document": {"type": "label", "id": "only"}})")),
            R"("text":"loaded")"));
  check(lv_obj_get_child_count(screen.raw()) == before + 1);
  const auto refused = agent.handle(call("11", "load_document", R"({"document": {"type": "chart"}})"));
  check(has(refused, R"("isError":true)") && has(refused, "unknown_widget: chart"));
  const auto bad_value = agent.handle(call("12", "apply_patch", R"({"patch": {"only": {"text": 5}}})"));
  check(has(bad_value, R"("isError":true)") && has(bad_value, "bad_value: 5"));
  check(has(agent.handle(call("13", "dump_tree", "{}")), R"(\"id\":\"only\")"));
  check(lv_obj_get_child_count(screen.raw()) == before + 1);

  // Arguments a tool cannot use are the tool's error, which the agent can read.
  for (const auto& [tool, arguments] :
       {std::pair{"load_document", R"({})"}, std::pair{"load_document", R"({"document": "x"})"},
        std::pair{"apply_patch", R"({})"}, std::pair{"apply_patch", R"({"patch": 5})"},
        std::pair{"screenshot", R"({"name": 5})"}, std::pair{"screenshot", R"({"name": ""})"}}) {
    const auto reply = agent.handle(call("14", tool, arguments));
    check(one_json_line(reply) && has(reply, R"("isError":true)") && !has(reply, R"("error")"));
  }

  // A client names a file, never a place: nothing but a plain *.png name is taken.
  const auto entries = [&] {
    return std::distance(std::filesystem::directory_iterator{dir}, std::filesystem::directory_iterator{});
  };
  const auto files = entries();
  for (const char* name :
       {"/tmp/agent_kit_x.png", "../agent_kit_x.png", "a/b.png", "x.txt", "x.png.txt", "x", ".png",
        ".hidden.png", "..", ".", "a b.png", "a\\b.png", "a\\u002fb.png", "C:x.png", "~x.png",
        "x.png/", "x.PNG", "a\tb.png", "$HOME.png", "*.png"}) {
    const auto reply = agent.handle(call("15", "screenshot", std::string{R"({"name": ")"} + name + R"("})"));
    check(one_json_line(reply) && has(reply, R"("isError":true)") && has(reply, "must be a file name"));
  }
  const auto too_long = agent.handle(
      call("15", "screenshot", R"({"name": ")" + std::string(125, 'n') + R"(.png"})"));
  check(has(too_long, R"("isError":true)") && has(too_long, "must be a file name"));
  check(entries() == files && !std::filesystem::exists("/tmp/agent_kit_x.png"));
  // The longest name, with every kind of character a name may have.
  const std::string longest = "aZ09_-." + std::string(117, 'n') + ".png";
  std::filesystem::remove(dir + "/" + longest);
  check(has(agent.handle(call("15", "screenshot", R"({"name": ")" + longest + R"("})")), "wrote "));
  check(std::filesystem::remove(dir + "/" + longest));

  // A directory that is not there.
  server lost{screen, {.screenshot_directory = "/no/such/dir"}};
  check(has(lost.handle(call("15", "screenshot", R"({"name": "x.png"})")), "cannot_write"));

  // By default a server writes no files. It still returns pictures.
  server careful{screen};
  const auto no_file = careful.handle(call("16", "screenshot", R"({"name": "agent_kit_never.png"})"));
  check(has(no_file, R"("isError":true)") && has(no_file, "does not write files"));
  check(!std::filesystem::exists("agent_kit_never.png") && entries() == files);
  check(has(careful.handle(call("17", "screenshot", "{}")), R"("type":"image")"));
  check(has(careful.handle(R"({"jsonrpc":"2.0","id":1,"method":"initialize"})"),
            R"("serverInfo":{"name":"lvgl26","version":"0.0.1"})"));
}

// A client of the 2026-07-28 revision: no handshake, every request says who asks.
void requests_with_their_own_metadata(lv::obj screen) {
  server agent{screen};
  constexpr std::string_view meta =
      R"("_meta": {"io.modelcontextprotocol/protocolVersion": "2026-07-28",
                   "io.modelcontextprotocol/clientCapabilities": {}})";
  const auto request = [&](std::string_view method, std::string_view more = {}) {
    std::string m = R"({"jsonrpc": "2.0", "id": "r", "method": ")";
    m += method;
    m += R"(", "params": {)";
    m += more;
    m += meta;
    m += "}}";
    return agent.handle(m);
  };

  const auto discovered = request("server/discover");
  check(one_json_line(discovered));
  const auto result = lv::json::value{discovered}["result"];
  check(result["resultType"].string() == "complete");
  check(result["supportedVersions"].size() == 1 && result["supportedVersions"].at(0).string() == "2026-07-28");
  check(result["capabilities"]["tools"].is(lv::json::kind::object));
  check(result["_meta"]["io.modelcontextprotocol/serverInfo"]["name"].string() == "lvgl26");
  check(result["instructions"].is(lv::json::kind::string));
  check(result["ttlMs"].text() == "0" && result["cacheScope"].string() == "public");

  const auto list_text = request("tools/list");
  const lv::json::value list{list_text};
  check(list["result"]["resultType"].string() == "complete" && list["result"]["tools"].size() == 4);
  check(list["result"]["ttlMs"].text() == "0" && list["result"]["cacheScope"].string() == "public");
  const auto loaded_text =
      request("tools/call", R"("name": "load_document", "arguments": {"document": {"type": "label"}}, )");
  const lv::json::value loaded{loaded_text};
  check(loaded["result"]["resultType"].string() == "complete");
  check(loaded["result"]["content"].at(0)["text"].string() == "loaded");
  check(loaded_text ==
        R"({"jsonrpc":"2.0","id":"r","result":{"resultType":"complete","_meta":)"
        R"({"io.modelcontextprotocol/serverInfo":{"name":"lvgl26","version":"0.0.1"}},)"
        R"("content":[{"type":"text","text":"loaded"}],"isError":false}})");

  // This revision has no handshake and no ping, and an unknown method is unknown.
  for (const char* method : {"initialize", "ping", "resources/list", ""}) {
    const auto gone_text = request(method);
    const lv::json::value gone{gone_text};
    check(one_json_line(gone_text) && gone["error"]["code"].number() == -32601);
    check(gone["error"]["message"].string() == "Method not found" && gone["id"].string() == "r");
  }
  // server/discover without the metadata is not a request of this revision.
  const auto bare_text = agent.handle(R"({"jsonrpc":"2.0","id":3,"method":"server/discover"})");
  const lv::json::value bare{bare_text};
  check(bare["error"]["code"].number() == -32602 && bare["id"].text() == "3");

  // Another revision is refused with the ones we speak; so is a version that is no text.
  const auto other = agent.handle(R"({"jsonrpc":"2.0","id":4,"method":"tools/list","params":{"_meta":
      {"io.modelcontextprotocol/protocolVersion":"2027-01-01","io.modelcontextprotocol/clientCapabilities":{}}}})");
  check(one_json_line(other));
  const auto error = lv::json::value{other}["error"];
  check(error["code"].number() == -32022 && error["message"].string() == "Unsupported protocol version");
  check(error["data"]["supported"].at(0).string() == "2026-07-28");
  check(error["data"]["requested"].string() == "2027-01-01" && lv::json::value{other}["id"].text() == "4");
  const auto number_text = agent.handle(
      R"({"jsonrpc":"2.0","id":4,"method":"tools/list","params":{"_meta":{"io.modelcontextprotocol/protocolVersion":7}}})");
  const lv::json::value number{number_text};
  check(number["error"]["code"].number() == -32022 && number["error"]["data"]["requested"].string().empty());
  // The capabilities are required.
  const auto no_capabilities_text = agent.handle(
      R"({"jsonrpc":"2.0","id":5,"method":"tools/list","params":{"_meta":{"io.modelcontextprotocol/protocolVersion":"2026-07-28"}}})");
  const lv::json::value no_capabilities{no_capabilities_text};
  check(no_capabilities["error"]["code"].number() == -32602 && no_capabilities["id"].text() == "5");
  // A notification of another revision is still not answered.
  check(agent.handle(R"({"jsonrpc":"2.0","method":"x","params":{"_meta":{"io.modelcontextprotocol/protocolVersion":"1"}}})").empty());
}

// Whatever arrives, the answer is a JSON-RPC error on one line.
void refuses(lv::obj screen) {
  server agent{screen};
  const auto expect = [&](std::string_view message, int code, std::string_view id) {
    const auto reply = agent.handle(message);
    check(one_json_line(reply));
    const lv::json::value r{reply};
    check(r["jsonrpc"].string() == "2.0" && r["error"]["code"].number() == code);
    check(r["id"].text() == id && r["error"]["message"].is(lv::json::kind::string) && !r["result"]);
  };
  expect("", -32700, "null");
  expect("{", -32700, "null");
  expect(R"({"jsonrpc": "2.0", "id": 1, "method": "ping"} x)", -32700, "null");
  expect("[]", -32600, "null");
  expect("5", -32600, "null");
  expect(R"([{"jsonrpc": "2.0", "id": 1, "method": "ping"}])", -32600, "null");  // no batches
  expect(R"({"id": 1, "method": "ping"})", -32600, "1");
  expect(R"({"jsonrpc": "1.0", "id": 1, "method": "ping"})", -32600, "1");
  expect(R"({"jsonrpc": 2.0, "id": 1, "method": "ping"})", -32600, "1");
  expect(R"({"jsonrpc": "2.0", "id": 1})", -32600, "1");
  expect(R"({"jsonrpc": "2.0", "id": 1, "method": 5})", -32600, "1");
  expect(R"({"jsonrpc": "2.0", "id": 1, "result": {}})", -32600, "1");  // a response is not ours to get
  expect(R"({"jsonrpc": "2.0"})", -32600, "null");
  // An id is a string or a number.
  expect(R"({"jsonrpc": "2.0", "id": null, "method": "ping"})", -32600, "null");
  expect(R"({"jsonrpc": "2.0", "id": {}, "method": "ping"})", -32600, "null");
  expect(R"({"jsonrpc": "2.0", "id": [1], "method": "ping"})", -32600, "null");
  expect(R"({"jsonrpc": "2.0", "id": true, "method": "ping"})", -32600, "null");
  expect("{\"jsonrpc\": \"2.0\", \"id\": \"a\nb\", \"method\": \"ping\"}", -32600, "null");
  expect("{\"jsonrpc\": \"2.0\", \"id\": \"a\x1f\", \"method\": \"ping\"}", -32600, "null");
  check(agent.handle("{\"jsonrpc\":\"2.0\",\"id\":\"a b\",\"method\":\"ping\"}") ==
        R"({"jsonrpc":"2.0","id":"a b","result":{}})");
  check(agent.handle(R"({"jsonrpc":"2.0","id":-1.5e3,"method":"ping"})") ==
        R"({"jsonrpc":"2.0","id":-1.5e3,"result":{}})");
  check(agent.handle(R"({"jsonrpc":"2.0","id":"q\"é","method":"ping"})") ==
        R"({"jsonrpc":"2.0","id":"q\"é","result":{}})");

  expect(R"({"jsonrpc": "2.0", "id": 2, "method": "resources/list"})", -32601, "2");
  expect(R"({"jsonrpc": "2.0", "id": 2, "method": ""})", -32601, "2");
  expect(R"({"jsonrpc": "2.0", "id": 3, "method": "tools/call"})", -32602, "3");
  expect(R"({"jsonrpc": "2.0", "id": 3, "method": "tools/call", "params": [1]})", -32602, "3");
  expect(R"({"jsonrpc": "2.0", "id": 3, "method": "tools/call", "params": {"name": 5}})", -32602, "3");
  expect(R"({"jsonrpc": "2.0", "id": 3, "method": "tools/call", "params": {"name": "dump_tree", "arguments": []}})",
         -32602, "3");
  expect(R"({"jsonrpc": "2.0", "id": 3, "method": "tools/call", "params": {"name": "dump_tree", "arguments": "x"}})",
         -32602, "3");
  expect(call("4", "format_disk", "{}"), -32602, "4");
  check(has(agent.handle(call("4", "format_disk", "{}")), R"("message":"Unknown tool: format_disk")"));
  // A name with a line feed in it comes back on one line.
  expect(call("4", "a\nb", "{}"), -32602, "4");
  check(has(agent.handle(call("4", "a\nb", "{}")), R"(Unknown tool: a\nb)"));

  // What a refused document is about is cut short, and escaped.
  const auto long_error = agent.handle(call("5", "load_document",
      R"({"document": {"type": "label", "props": {"text": [")" + std::string(500, 'x') + "\"]}}}"));
  check(one_json_line(long_error) && has(long_error, R"(bad_value: [\")" + std::string(198, 'x') + "\"}"));
  check(!has(long_error, std::string(199, 'x')));
}

constexpr std::string_view replacement = "\xef\xbf\xbd";  // U+FFFD in UTF-8

// A strict JSON client rejects a reply that is not UTF-8, so text from the client
// goes back well-formed: a character intact, a byte that is no character replaced.
void replies_are_well_formed_utf8(lv::obj screen) {
  server agent{screen};
  const auto message_of = [&](std::string_view reply) {
    return std::string{lv::json::value{reply}["error"]["message"].string()};
  };
  // A valid emoji in a tool name echoes intact; an invalid byte becomes the replacement.
  const auto emoji = agent.handle(call("4", "\xf0\x9f\x98\x80", "{}"));
  check(one_json_line(emoji) && message_of(emoji) == "Unknown tool: \xf0\x9f\x98\x80");
  const auto broken = agent.handle(call("4", "x\xff\xc3", "{}"));
  check(one_json_line(broken) &&
        message_of(broken) == "Unknown tool: x" + std::string{replacement} + std::string{replacement});
  // An id goes back as written, so one that is not UTF-8 is refused.
  const auto bad_id = agent.handle("{\"jsonrpc\":\"2.0\",\"id\":\"a\xff\",\"method\":\"ping\"}");
  check(one_json_line(bad_id) && lv::json::value{bad_id}["error"]["code"].number() == -32600);
  check(lv::json::value{bad_id}["id"].text() == "null" && !has(bad_id, "\xff"));
  // An excerpt is cut before a character, not inside it: here 197 x and a euro sign
  // that would straddle the cut.
  const auto cut = agent.handle(call("5", "load_document",
      R"({"document": {"type": "label", "props": {"text": [")" + std::string(197, 'x') +
          "\xe2\x82\xac" + std::string(300, 'y') + "\"]}}}"));
  check(one_json_line(cut) && has(cut, R"(bad_value: [\")" + std::string(197, 'x') + "\"}"));
  check(!has(cut, "\xe2") && !has(cut, replacement));
}

// A client cannot fill the directory the host allows: a server writes a limited
// number of screenshot files, and then refuses with a reason the agent can read.
void screenshot_files_stop_at_the_limit(lv::obj screen, const std::string& dir) {
  const std::string shots = dir + "/screenshot_limit";
  std::filesystem::remove_all(shots);
  std::filesystem::create_directory(shots);
  const auto limit = lv::mcp::detail::screenshot_file_limit;
  {
    server agent{screen, {.screenshot_directory = shots}};
    // A refused name does not count.
    check(has(agent.handle(call("1", "screenshot", R"({"name": "x.txt"})")), "must be a file name"));
    for (std::size_t i = 0; i < limit; ++i)
      check(has(agent.handle(call("1", "screenshot", R"({"name": "shot)" + std::to_string(i) + R"(.png"})")),
                R"("isError":false)"));
    const auto refused = agent.handle(call("2", "screenshot", R"({"name": "one_more.png"})"));
    check(one_json_line(refused) && has(refused, R"("isError":true)"));
    check(has(refused, "has written its " + std::to_string(limit) + " screenshot files"));
    // A picture without a file is still returned.
    check(has(agent.handle(call("3", "screenshot", "{}")), R"("type":"image")"));
  }
  const auto files = std::distance(std::filesystem::directory_iterator{shots}, std::filesystem::directory_iterator{});
  check(files == static_cast<std::ptrdiff_t>(limit) && !std::filesystem::exists(shots + "/one_more.png"));
  std::filesystem::remove_all(shots);
}

// While it lives, no file of this process can grow past `bytes`: a write past that
// fails (EFBIG) instead of ending the process (SIGXFSZ).
struct file_size_limit {
  rlimit before{};
  void (*handler)(int) = std::signal(SIGXFSZ, SIG_IGN);
  explicit file_size_limit(rlim_t bytes) {
    getrlimit(RLIMIT_FSIZE, &before);
    const rlimit lower{.rlim_cur = bytes, .rlim_max = before.rlim_max};
    setrlimit(RLIMIT_FSIZE, &lower);
  }
  file_size_limit(const file_size_limit&) = delete;
  file_size_limit& operator=(const file_size_limit&) = delete;
  ~file_size_limit() {
    setrlimit(RLIMIT_FSIZE, &before);
    std::signal(SIGXFSZ, handler);
  }
};

// A screenshot whose file was made but could not be written still counts: the file
// stays in the directory, so the limit on files must hold for it too.
void screenshot_files_that_fail_to_write_count_to_the_limit(lv::obj screen, const std::string& dir) {
  const std::string shots = dir + "/screenshot_limit_failed";
  std::filesystem::remove_all(shots);
  std::filesystem::create_directory(shots);
  const auto limit = lv::mcp::detail::screenshot_file_limit;
  {
    server agent{screen, {.screenshot_directory = shots}};
    {
      const file_size_limit full{0};
      for (std::size_t i = 0; i < limit; ++i)
        check(has(agent.handle(call("1", "screenshot", R"({"name": "shot)" + std::to_string(i) + R"(.png"})")),
                  "cannot_write"));
    }
    const auto refused = agent.handle(call("2", "screenshot", R"({"name": "one_more.png"})"));
    check(has(refused, R"("isError":true)") && has(refused, "screenshot files"));
  }
  const auto files = std::distance(std::filesystem::directory_iterator{shots}, std::filesystem::directory_iterator{});
  check(files == static_cast<std::ptrdiff_t>(limit) && !std::filesystem::exists(shots + "/one_more.png"));
  std::filesystem::remove_all(shots);
}

// The loop over a stream: one answer per request, none for the rest.
void lines() {
  std::FILE* in = std::tmpfile();
  std::FILE* out = std::tmpfile();
  check(in != nullptr && out != nullptr);
  if (in == nullptr || out == nullptr) return;
  std::fputs("{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"ping\"}\n"
             "\n"
             " \t\r\n"
             "{\"jsonrpc\":\"2.0\",\"method\":\"notifications/initialized\"}\n"
             "nonsense\r\n"
             "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"ping\"}",  // the last line has no line feed
             in);
  std::rewind(in);
  server agent{lv::screen_active()};
  agent.serve(in, out);
  std::rewind(out);
  std::string written;
  for (int c = std::fgetc(out); c != EOF; c = std::fgetc(out)) written.push_back(static_cast<char>(c));
  check(written ==
        "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":{}}\n"
        "{\"jsonrpc\":\"2.0\",\"id\":null,\"error\":{\"code\":-32700,\"message\":\"Parse error: not JSON\"}}\n"
        "{\"jsonrpc\":\"2.0\",\"id\":2,\"result\":{}}\n");
  std::fclose(in);
  std::fclose(out);

  // An empty stream ends at once.
  std::FILE* nothing = std::tmpfile();
  std::FILE* silence = std::tmpfile();
  if (nothing != nullptr && silence != nullptr) {
    agent.serve(nothing, silence);
    check(std::ftell(silence) == 0);
  }
  if (nothing != nullptr) std::fclose(nothing);
  if (silence != nullptr) std::fclose(silence);
}

// What serve() writes for `input`, read through a temporary file as from stdin.
std::string served(server& agent, std::string_view input) {
  std::FILE* in = std::tmpfile();
  std::FILE* out = std::tmpfile();
  check(in != nullptr && out != nullptr);
  std::string written;
  if (in != nullptr && out != nullptr) {
    std::fwrite(input.data(), 1, input.size(), in);
    std::rewind(in);
    agent.serve(in, out);
    std::rewind(out);
    for (int c = std::fgetc(out); c != EOF; c = std::fgetc(out)) written.push_back(static_cast<char>(c));
  }
  if (in != nullptr) std::fclose(in);
  if (out != nullptr) std::fclose(out);
  return written;
}

constexpr std::string_view ping_one = R"({"jsonrpc":"2.0","id":1,"method":"ping"})";
constexpr std::string_view pong_one = "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":{}}\n";

// `message` padded with spaces to `size` bytes: still the same JSON.
std::string padded(std::string_view message, std::size_t size) {
  std::string line{message};
  line.resize(size, ' ');
  return line;
}

// A client that never ends a line cannot make the server hold all it sends: a line
// over the limit is dropped up to its line feed and answered with one error, and the
// session goes on.
void a_line_over_the_limit_is_refused_and_the_session_goes_on() {
  server agent{lv::screen_active()};
  const auto limit = lv::mcp::detail::line_limit;
  const std::string refused =
      "{\"jsonrpc\":\"2.0\",\"id\":null,\"error\":{\"code\":-32600,\"message\":"
      "\"Invalid Request: a message is longer than " + std::to_string(limit) + " bytes\"}}\n";

  // A line exactly at the limit is read, with a line feed or at the end of the stream.
  check(served(agent, padded(ping_one, limit) + "\n") == pong_one);
  check(served(agent, padded(ping_one, limit)) == pong_one);
  // One byte more is refused, and the next request is answered.
  check(served(agent, padded(ping_one, limit + 1) + "\n" + std::string{ping_one} + "\n") ==
        refused + std::string{pong_one});
  // A line that never ends gets its one error, then the stream ends.
  check(served(agent, std::string(limit * 2, 'x')) == refused);
  // A tool call after an oversize line still works.
  const std::string after = served(agent, std::string(limit + 1, '{') + "\n" + call("2", "dump_tree", "{}") + "\n");
  check(after.starts_with(refused) && has(after, R"("id":2,"result":{"content":[{"type":"text")"));
}

// The line reader takes bytes from anything that gives them, here a text in memory,
// and keeps no more than the limit of a line, whatever comes.
void the_line_reader_keeps_at_most_the_limit() {
  using lv::mcp::detail::line_status;
  constexpr std::size_t limit = 4;
  std::string_view input = "abcd\nabcde\n\nab\r\nabcdefgh";
  auto next = [&input] {
    if (input.empty()) return EOF;
    const auto c = static_cast<unsigned char>(input.front());
    input.remove_prefix(1);
    return static_cast<int>(c);
  };
  std::string line;
  const auto read = [&](line_status expected, std::string_view text) {
    check(lv::mcp::detail::read_line(next, line, limit) == expected && line.size() <= limit);
    check(expected == line_status::too_long || line == text);
  };
  read(line_status::line, "abcd");
  read(line_status::too_long, "");
  read(line_status::line, "");
  read(line_status::line, "ab\r");
  read(line_status::too_long, "");
  read(line_status::end, "");
  read(line_status::end, "");
}

void base64() {
  const auto encode = [](std::string_view text) {
    std::string out;
    lv::mcp::detail::append_base64(
        out, {reinterpret_cast<const std::uint8_t*>(text.data()), text.size()});
    return out;
  };
  check(encode(std::string{}) == "");
  check(encode(std::string{"f"}) == "Zg==" && encode(std::string{"fo"}) == "Zm8=");
  check(encode(std::string{"foo"}) == "Zm9v" && encode(std::string{"foob"}) == "Zm9vYg==");
  check(encode(std::string{"\xfb\xff\xfe"}) == "+//+" && encode(std::string{"\x00\x10\x83", 3}) == "ABCD");
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2) {
    std::printf("usage: agent_kit <directory to write pictures to>\n");
    return 2;
  }
  const std::string dir = argv[1];

  lv_init();
  lv_test_display_create(320, 240);
  auto screen = lv::screen_active();

  json_text();
  the_writer_writes_only_well_formed_utf8();
  values();
  base64();
  tree_dump(screen);
  pictures(screen, dir);
  picture_runs(screen, dir);
  agent_changes_the_screen(screen, dir);
  requests_with_their_own_metadata(screen);
  refuses(screen);
  replies_are_well_formed_utf8(screen);
  screenshot_files_stop_at_the_limit(screen, dir);
  screenshot_files_that_fail_to_write_count_to_the_limit(screen, dir);
  lines();
  a_line_over_the_limit_is_refused_and_the_session_goes_on();
  the_line_reader_keeps_at_most_the_limit();

  lv_deinit();
  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
