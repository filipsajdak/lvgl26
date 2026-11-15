#pragma once

// A widget tree written as JSON, so that an agent or a test can see what is on
// the screen without looking at pixels.
//
//   using ui = lv::runtime<"obj", "label", "arc">;
//   std::string text;
//   ui::dump(lv::screen_active(), [&](std::string_view piece) { text += piece; });
//
//   {"type":"obj","x":0,"y":0,"width":320,"height":240,"props":{...},"children":[
//     {"type":"arc","id":"speedo","x":105,"y":65,"width":110,"height":110,
//      "props":{"value":120,"rotation":90,...}}]}
//
// Per widget: its type, its id if it came from a document that gave it one, its
// place on the screen in pixels, and its properties. "type" is null for a widget
// that is not one of the listed kinds; it then has the properties of obj.
//
// A property is listed when reflection finds the getter of its setter:
// lv_arc_get_value for lv_arc_set_value. The names are those of lv::props and of
// a runtime document. Numbers are plain, in LVGL's own units; an enumerator is
// written by its full name; a color as "#rrggbb". Styles are not listed.
//
// The text goes to a sink (see lv/json_writer.hpp), piece by piece. Nothing is
// allocated and nothing is thrown.

#include <lv/detail/api.hpp>
#include <lv/detail/enum_names.hpp>
#include <lv/detail/fixed_string.hpp>
#include <lv/json_writer.hpp>
#include <lv/props.hpp>

#include <array>
#include <concepts>
#include <cstdint>
#include <memory>
#include <span>
#include <string_view>
#include <type_traits>

namespace lv::detail {

// ---- where the text goes -------------------------------------------------------

// A sink behind two pointers, so that the tree walk is not a template.
struct text_out {
  void* context;
  void (*write)(void*, std::string_view);
  void operator()(std::string_view piece) const { write(context, piece); }
};

template <class S>
text_out erase_sink(S& sink) {
  return {const_cast<void*>(static_cast<const void*>(std::addressof(sink))),
          [](void* context, std::string_view piece) { std::invoke(*static_cast<S*>(context), piece); }};
}

// ---- writing values ------------------------------------------------------------

// write_value(out, v): one overload per kind of value a getter returns.

inline void write_value(text_out out, bool v) { out(v ? "true" : "false"); }

template <std::integral I>
void write_value(text_out out, I v) {
  json::write_number(out, static_cast<std::int64_t>(v));
}

// An enumerator by its name. A value that is none of them (several flags) is a number.
template <class E>
  requires std::is_enum_v<E>
void write_value(text_out out, E v) {
  for (const auto& e : enum_names<E>)
    if (e.value == static_cast<std::int64_t>(v)) return json::write_string(out, e.name);
  json::write_number(out, static_cast<std::int64_t>(v));
}

inline void write_value(text_out out, const char* v) {
  if (v == nullptr) return out("null");
  json::write_string(out, v);
}

inline void write_value(text_out out, lv_color_t v) {
  constexpr std::string_view digits = "0123456789abcdef";
  std::array<char, 9> text{'"', '#', 0, 0, 0, 0, 0, 0, '"'};
  std::size_t at = 2;
  for (const std::uint8_t part : {v.red, v.green, v.blue}) {
    text[at++] = digits[part >> 4];
    text[at++] = digits[part & 15];
  }
  out(std::string_view{text.data(), text.size()});
}

// Whether a getter's return type has a write_value overload.
consteval bool writable(std::meta::info type) {
  const auto t = std::meta::remove_cv(std::meta::dealias(type));
  return std::meta::is_integral_type(t) || std::meta::is_enum_type(t) || t == ^^const char* ||
         t == ^^char* || t == std::meta::dealias(^^lv_color_t);
}

// ---- the getters of a widget ---------------------------------------------------

// lv_<widget>_get_<prop> for the setter lv_<widget>_set_<prop>, if it takes only
// the widget and returns something that can be written. A null reflection otherwise.
consteval std::meta::info getter_of(std::meta::info setter) {
  const auto name = std::meta::identifier_of(setter);
  const auto at = name.find("_set_");
  if (at == std::string_view::npos) return {};  // lv_obj_add_style
  const auto f = find(str(name.substr(0, at)) + "_get_" + str(name.substr(at + 5)));
  if (f == std::meta::info{}) return f;
  const auto params = std::meta::parameters_of(f);
  if (params.size() != 1) return {};
  const auto widget = std::meta::dealias(std::meta::type_of(params[0]));
  constexpr auto plain = ^^lv_obj_t*;
  constexpr auto constant = ^^const lv_obj_t*;
  if (widget != plain && widget != constant) return {};
  return writable(std::meta::return_type_of(f)) ? f : std::meta::info{};
}

// The readable properties of widget W as pairs: the name, then the getter.
template <fixed W>
consteval std::vector<std::meta::info> scan_getters() {
  std::vector<std::meta::info> out;
  for (auto target : targets<W>) {
    if (target == std::meta::info{} || !std::meta::is_function(target)) continue;
    const auto getter = getter_of(target);
    if (getter == std::meta::info{}) continue;
    const auto name = std::meta::identifier_of(target);
    out.push_back(std::meta::reflect_constant_string(name.substr(name.find("_set_") + 5)));
    out.push_back(getter);
  }
  return out;
}

template <fixed W>
inline constexpr std::span<const std::meta::info> getters =
    std::define_static_array(scan_getters<W>());

// Writes one property of a widget.
using write_fn = void (*)(lv_obj_t*, text_out);

struct readable_property {
  std::string_view name;
  write_fn write;
};

template <fixed W, std::size_t K>
void write_property(lv_obj_t* o, text_out out) {
  write_value(out, [:getters<W>[2 * K + 1]:](o));
}

// The properties of widget W that can be read, by name.
template <fixed W>
consteval auto make_getter_table() {
  std::array<readable_property, getters<W>.size() / 2> table{};
  template for (constexpr auto k : indices<getters<W>.size() / 2>) {
    table[k] = {std::meta::extract<const char*>(getters<W>[2 * k]), &write_property<W, k>};
  }
  return table;
}

template <fixed W>
inline constexpr auto getter_table = make_getter_table<W>();

// ---- which widget an object is -------------------------------------------------

// Every lv_<widget>_class variable of LVGL. A template so that the scan of the
// global namespace only runs in a file that dumps a tree.
template <class D>
consteval std::vector<std::meta::info> scan_classes() {
  std::vector<std::meta::info> out;
  for (auto m : std::meta::members_of(^^::, std::meta::access_context::unchecked()))
    if (std::meta::is_variable(m) && std::meta::has_identifier(m) &&
        std::meta::remove_cv(std::meta::dealias(std::meta::type_of(m))) ==
            std::meta::dealias(^^lv_obj_class_t))
      out.push_back(m);
  return out;
}

template <class D = void>
inline constexpr std::span<const std::meta::info> classes =
    std::define_static_array(scan_classes<D>());

// The variable lv_<widget>_class, or a null reflection: LVGL does not publish the
// class of every widget.
template <class D = void>
consteval std::meta::info class_of(std::string_view widget) {
  const std::string name = "lv_" + str(widget) + "_class";
  for (auto c : classes<D>)
    if (std::meta::identifier_of(c) == name) return c;
  return {};
}

template <fixed W>
constexpr const lv_obj_class_t* class_pointer() {
  constexpr auto c = class_of(W.sv());
  if constexpr (c == std::meta::info{})
    return nullptr;
  else
    return &[:c:];
}

struct widget_entry {
  std::string_view name;
  const lv_obj_class_t* cls;  // null when LVGL does not publish it
  std::span<const readable_property> properties;
};

template <fixed... Widgets>
inline constexpr std::array<widget_entry, sizeof...(Widgets)> widget_table{
    widget_entry{Widgets.sv(), class_pointer<Widgets>(), getter_table<Widgets>}...};

// ---- the walk ------------------------------------------------------------------

// Finds the id of a widget, if something knows it. Empty when nothing does.
struct id_lookup {
  const void* context = nullptr;
  std::string_view (*find)(const void*, lv_obj_t*) = nullptr;
  std::string_view operator()(lv_obj_t* o) const {
    return find != nullptr ? find(context, o) : std::string_view{};
  }
};

struct tree_dump {
  text_out out;
  std::span<const widget_entry> widgets;
  std::span<const readable_property> base;  // of obj: for a widget that is not listed
  id_lookup ids;

  // The listed kind whose class this object has, or null.
  const widget_entry* entry_of(lv_obj_t* o) const {
    const lv_obj_class_t* cls = lv_obj_get_class(o);
    for (const auto& w : widgets)
      if (w.cls == cls) return &w;
    return nullptr;
  }

  void number(std::string_view key, std::int64_t value) const {
    out(key);
    json::write_number(out, value);
  }

  void type_and_id(lv_obj_t* o, const widget_entry* entry) const {
    out("{\"type\":");
    if (entry != nullptr)
      json::write_string(out, entry->name);
    else
      out("null");
    if (const auto id = ids(o); !id.empty()) {
      out(",\"id\":");
      json::write_string(out, id);
    }
  }

  // Where the widget is on the screen, in pixels.
  void box(lv_obj_t* o) const {
    lv_area_t area;
    lv_obj_get_coords(o, &area);
    number(",\"x\":", area.x1);
    number(",\"y\":", area.y1);
    number(",\"width\":", std::int64_t{area.x2} - area.x1 + 1);
    number(",\"height\":", std::int64_t{area.y2} - area.y1 + 1);
  }

  void properties(lv_obj_t* o, std::span<const readable_property> list) const {
    out(",\"props\":{");
    for (std::size_t i = 0; i < list.size(); ++i) {
      if (i > 0) out(",");
      json::write_string(out, list[i].name);
      out(":");
      list[i].write(o, out);
    }
    out("}");
  }

  void children(lv_obj_t* o) const {
    const std::uint32_t count = lv_obj_get_child_count(o);
    if (count == 0) return;
    out(",\"children\":[");
    for (std::uint32_t i = 0; i < count; ++i) {
      if (i > 0) out(",");
      node(lv_obj_get_child(o, static_cast<std::int32_t>(i)));
    }
    out("]");
  }

  void node(lv_obj_t* o) const {
    const widget_entry* entry = entry_of(o);
    type_and_id(o, entry);
    box(o);
    properties(o, entry != nullptr ? entry->properties : base);
    children(o);
    out("}");
  }
};

// Writes the tree under `root`, whose widgets are of the kinds Widgets..., to `sink`.
template <fixed... Widgets, json::sink S>
void dump_tree(lv_obj_t* root, S& sink, id_lookup ids) {
  lv_obj_update_layout(root);  // so that the places are those of the next frame
  tree_dump{erase_sink(sink), widget_table<Widgets...>, getter_table<"obj">, ids}.node(root);
}

}  // namespace lv::detail
