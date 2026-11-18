// Which angles are whole degrees and which are tenths of a degree.
//
// lvgl26 reads that from LVGL's doc comments (see lv/detail/angle_docs.hpp). This
// test checks the reading in three ways:
//  - static assertions on what the comments say for setters we know;
//  - a static assertion that no function named after an angle is declared in a
//    header that was not read;
//  - at run time, by what LVGL does. An angle is periodic: a widget drawn at v and
//    at v + 360 degrees looks the same. So every angle setter of every widget and of
//    the style is called with v, v + 360 and v + 3600, and the pixels say whether
//    its unit is a degree or a tenth of one. The test fails when the pixels and the
//    comment disagree. A setter whose effect cannot be seen is named in the output,
//    and must be on the list below: a new one fails the test until it is looked at.
#include <lv/lv.hpp>

#include "check.hpp"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <meta>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

using namespace lv::units;
namespace d = lv::detail;
using std::string_view;

// ---- what the doc comments say ---------------------------------------------------

// In tenths of a degree: the rotation of an image, and the three transform angles
// of a style. Their getters too.
static_assert(d::angle_type_of(^^lv_image_set_rotation) == ^^lv::decidegrees);
static_assert(d::angle_type_of(^^lv_obj_set_style_transform_rotation) == ^^lv::decidegrees);
static_assert(d::angle_type_of(^^lv_obj_set_style_transform_skew_x) == ^^lv::decidegrees);
static_assert(d::angle_type_of(^^lv_obj_set_style_transform_skew_y) == ^^lv::decidegrees);
static_assert(d::angle_type_of(^^lv_image_get_rotation) == ^^lv::decidegrees);
static_assert(d::angle_type_of(^^lv_obj_get_style_transform_rotation) == ^^lv::decidegrees);
static_assert(d::angle_type_of(^^lv_obj_get_style_transform_skew_x) == ^^lv::decidegrees);
static_assert(d::angle_type_of(^^lv_obj_get_style_transform_skew_y) == ^^lv::decidegrees);

// In whole degrees: everything else.
static_assert(d::angle_type_of(^^lv_arc_set_rotation) == ^^lv::angle);
static_assert(d::angle_type_of(^^lv_arc_set_start_angle) == ^^lv::angle);
static_assert(d::angle_type_of(^^lv_arc_set_bg_angles) == ^^lv::angle);
static_assert(d::angle_type_of(^^lv_scale_set_rotation) == ^^lv::angle);
static_assert(d::angle_type_of(^^lv_scale_set_angle_range) == ^^lv::angle);
static_assert(d::angle_type_of(^^lv_spinner_set_arc_sweep) == ^^lv::angle);
static_assert(d::angle_type_of(^^lv_arc_get_rotation) == ^^lv::angle);
static_assert(d::angle_type_of(^^lv_scale_get_rotation) == ^^lv::angle);

// The eight above are all that LVGL v9.6 and the v10 pin have in tenths.
static_assert(d::tenths_functions<>.size() == 8);

// So the properties get these types.
static_assert(std::is_same_v<decltype(lv::props<"image">{}.rotation), lv::arg<lv::decidegrees>>);
static_assert(std::is_same_v<decltype(lv::props<"arc">{}.rotation), lv::arg<lv::angle>>);
static_assert(std::is_same_v<decltype(lv::props<"arc">{}.bg_angles), lv::arg<lv::angle, lv::angle>>);
static_assert(std::is_same_v<decltype(lv::props<"scale">{}.rotation), lv::arg<lv::angle>>);
using style_type = d::style_t<>;
static_assert(std::is_same_v<decltype(style_type{}.transform_rotation),
                             lv::arg<lv::styled<lv::decidegrees>>>);
static_assert(std::is_same_v<decltype(style_type{}.transform_skew_x),
                             lv::arg<lv::styled<lv::decidegrees>>>);
static_assert(std::is_same_v<decltype(style_type{}.transform_skew_y),
                             lv::arg<lv::styled<lv::decidegrees>>>);
// A scale is a ratio, not an angle.
static_assert(std::is_same_v<decltype(style_type{}.transform_scale_x),
                             lv::arg<lv::styled<lv::strict<std::int32_t>>>>);

// ---- the search of a header -------------------------------------------------------

consteval std::vector<string_view> in_tenths(string_view text) {
  std::vector<string_view> out;
  d::collect_tenths(text, out);
  return out;
}
consteval bool finds(string_view text, std::initializer_list<string_view> names) {
  return std::ranges::equal(in_tenths(text), names);
}

// The function declared after a comment with the mark, whatever its return type.
static_assert(finds("/** in 0.1 degree units */ void set_a(int v);", {"set_a"}));
static_assert(finds("/**\n * 0.1 degrees\n */\nint32_t * get_b(void);", {"get_b"}));
static_assert(finds("/** 0.1 degree */\nvoid*set_c(int);", {"set_c"}));
// Only the function that follows the comment, and each function once per mention.
static_assert(finds("/** degrees */ void set_a(int);\n/** 0.1 degree */ void set_b(int);\n"
                    "/** degrees */ void set_c(int);",
                    {"set_b"}));
static_assert(finds("/** 0.1 degree */ void a(int); /** 0.1 degree */ void b(int);", {"a", "b"}));
// Other numbers with a 1, and the mark cut short, are not the mark.
static_assert(finds("/** 1 degree, 10.1 degrees, x0.1 degree, 0.1 deg, 0.1degree */ void a(int);", {}));
static_assert(finds("/** (0.1 degree) */ void a(int);", {"a"}));
static_assert(finds("1", {}) && finds("0.1", {}) && finds(".1 degree */ void a(int);", {}));
static_assert(finds("", {}));
// The mark at the very start of a text.
static_assert(finds("0.1 degree */ void a(int);", {"a"}));
// A mark with no declaration after it names nothing.
static_assert(finds("/** 0.1 degree", {}) && finds("/** 0.1 degree */ int x;", {}));
static_assert(finds("/** 0.1 degree */ (void);", {}));

static_assert(d::function_after("/* x */ void f(int);", 0) == "f");
static_assert(d::function_after("/* x */ void f(int); /* y */ int g();", 9) == "g");
static_assert(d::function_after("/* x */ void lv_obj_set_x2(int);", 3) == "lv_obj_set_x2");
static_assert(d::function_after("void f(int);", 0).empty());
static_assert(d::function_after("/* x */", 0).empty());
// A name at the very end of the text is not a function: no parenthesis follows.
static_assert(d::function_after("/* x */ int x", 0).empty());
static_assert(finds("/** 0.1 degree */ int x", {}));
// The name stops at the end of the comment, even with nothing between them.
static_assert(d::function_after("/* x */f(int);", 0) == "f");

// The table for a set of headers. One without the mark is a compile error: see
// tests/fail/angle_mark_not_found.cpp.
constexpr d::doc_header two_headers[] = {
    {"a.h", "/** degrees */ void set_a(int);"},
    {"b.h", "/** 0.1 degree */ void set_b(int); /** 0.1 degree */ int get_b(void);"}};
static_assert(d::functions_in_tenths(two_headers).size() == 2);
static_assert(string_view(d::functions_in_tenths(two_headers)[0]) == "set_b");
static_assert(string_view(d::functions_in_tenths(two_headers)[1]) == "get_b");

// A function is looked up by the header it is declared in.
static_assert(d::is_declared_in(^^lv_image_set_rotation, d::angle_headers));
static_assert(d::is_declared_in(^^lv_obj_set_style_transform_rotation, d::angle_headers));
static_assert(!d::is_declared_in(^^lv_label_set_text, d::angle_headers));
static_assert(!d::is_declared_in(^^lv_image_set_rotation, two_headers));
constexpr d::doc_header only_image[] = {{"lvgl/widgets/lv_image.h", ""}};
static_assert(d::is_declared_in(^^lv_image_set_rotation, only_image));
static_assert(!d::is_declared_in(^^lv_arc_set_rotation, only_image));

// ---- no angle is missed -----------------------------------------------------------

consteval bool is_number(std::meta::info type) {
  const auto t = std::meta::remove_cv(std::meta::dealias(type));
  return std::meta::is_arithmetic_type(t) && t != ^^bool;
}

// Whether `f` works on a widget: lv_<...>(lv_obj_t*, ...).
consteval bool takes_widget(std::meta::info f) {
  const auto params = std::meta::parameters_of(f);
  if (params.empty()) return false;
  const auto first = std::meta::remove_cv(std::meta::dealias(std::meta::type_of(params[0])));
  return first == ^^lv_obj_t* || first == ^^const lv_obj_t*;
}

// Whether a setter has a number that is named as an angle, by its own name or by the
// parameter's.
consteval bool sets_an_angle(std::meta::info f, string_view name) {
  const auto params = std::meta::parameters_of(f);
  for (std::size_t i = 1; i < params.size(); ++i) {
    const string_view parameter =
        std::meta::has_identifier(params[i]) ? std::meta::identifier_of(params[i]) : string_view{};
    if (is_number(std::meta::type_of(params[i])) && d::is_angle_name(name, parameter)) return true;
  }
  return false;
}

consteval bool has_an_angle(std::meta::info f) {
  const string_view name = std::meta::identifier_of(f);
  if (!takes_widget(f)) return false;
  if (d::has(name, "_set_")) return sets_an_angle(f, name);
  return d::has(name, "_get_") && is_number(std::meta::return_type_of(f)) &&
         d::is_angle_name(name, {});
}

// The setters and getters of widgets that are named after an angle and declared in a
// header whose doc comments were not read. There must be none.
consteval string_view angles_not_read() {
  std::string out;
  for (auto f : d::api)
    if (has_an_angle(f) && !d::is_declared_in(f, d::angle_headers))
      out += " " + d::str(std::meta::identifier_of(f)) + " (" +
             d::str(std::meta::source_location_of(f).file_name()) + ")";
  if (out.empty()) return {};
  return std::define_static_string(
      "These functions take or return an angle, and their headers are not in angle_headers "
      "of lv/detail/angle_docs.hpp:" + out);
}
static_assert(angles_not_read().empty(), angles_not_read());

// The check sees the functions it is for.
static_assert(has_an_angle(^^lv_image_set_rotation) && has_an_angle(^^lv_image_get_rotation));
static_assert(has_an_angle(^^lv_arc_set_bg_angles) && has_an_angle(^^lv_spinner_set_arc_sweep));
static_assert(has_an_angle(^^lv_obj_set_style_transform_skew_x));
static_assert(has_an_angle(^^lv_obj_get_style_transform_rotation));
static_assert(!has_an_angle(^^lv_label_set_text) && !has_an_angle(^^lv_obj_set_width));
static_assert(!has_an_angle(^^lv_display_set_rotation));  // an enum, and not a widget

// ---- what LVGL does ---------------------------------------------------------------

// One angle of one setter. `create` makes the widget; it is null for a style value,
// which is tried on a plain object.
struct target {
  std::meta::info setter;
  std::meta::info create;
  std::size_t parameter;
};

consteval bool is_angle(std::meta::info type) {
  return type == ^^lv::angle || type == ^^lv::decidegrees || type == ^^lv::styled<lv::angle> ||
         type == ^^lv::styled<lv::decidegrees> || type == ^^lv::nonnegative<lv::angle> ||
         type == ^^lv::nonnegative<lv::decidegrees>;
}

// lv_<widget>_create(parent), by "lv_<widget>".
struct maker {
  string_view prefix;
  std::meta::info create;
};
consteval std::vector<maker> makers() {
  std::vector<maker> out;
  for (auto f : d::api) {
    const auto name = std::meta::identifier_of(f);
    if (name.ends_with("_create") && std::meta::parameters_of(f).size() == 1 &&
        std::meta::return_type_of(f) == ^^lv_obj_t*)
      out.push_back({name.substr(0, name.size() - string_view("_create").size()), f});
  }
  return out;
}

consteval void add_style_target(std::vector<target>& out, std::meta::info f) {
  const auto params = std::meta::parameters_of(f);
  const auto prop = std::meta::identifier_of(f).substr(d::style_prefix.size());
  if (params.size() == 3 && is_angle(d::style_value_type(f, prop, params[1])))
    out.push_back({f, std::meta::info{}, 1});
}

consteval void add_widget_targets(std::vector<target>& out, std::meta::info f,
                                  const std::vector<maker>& widgets) {
  const auto name = std::meta::identifier_of(f);
  const auto at = name.find("_set_");
  if (at == string_view::npos) return;
  const auto prop = name.substr(at + string_view("_set_").size());
  const auto widget = std::ranges::find(widgets, name.substr(0, at), &maker::prefix);
  if (widget == widgets.end() || !d::is_usable_setter(f, prop)) return;
  const auto params = std::meta::parameters_of(f);
  for (std::size_t i = 1; i < params.size(); ++i)
    if (is_angle(d::value_type(f, prop, params[i]))) out.push_back({f, widget->create, i});
}

// Every angle that a property or a style value sets: the same setters, and the same
// rule for their types, as props<W> and lv::style use.
consteval std::vector<target> scan_targets() {
  const auto widgets = makers();
  std::vector<target> out;
  for (auto f : d::api) {
    if (std::meta::identifier_of(f).starts_with(d::style_prefix))
      add_style_target(out, f);
    else
      add_widget_targets(out, f, widgets);
  }
  return out;
}
inline constexpr auto targets = std::define_static_array(scan_targets());

// "lv_arc_set_bg_angles(start)".
template <target T>
inline constexpr const char* name_of = std::define_static_string(
    d::str(std::meta::identifier_of(T.setter)) + "(" +
    d::str(std::meta::identifier_of(std::meta::parameters_of(T.setter)[T.parameter])) + ")");

// The widgets and the three style values, at least.
static_assert(targets.size() >= 12);

// The pixels of the screen.
std::vector<std::uint8_t> shot() {
  lv_obj_update_layout(lv_screen_active());
  lv_draw_buf_t* b = lv_snapshot_take(lv_screen_active(), LV_COLOR_FORMAT_ARGB8888);
  if (b == nullptr) return {};
  std::vector<std::uint8_t> out(b->data, b->data + b->data_size);
  lv_draw_buf_destroy(b);
  return out;
}

// A picture that looks different from every side: the left half red, the right blue.
constexpr std::int32_t picture_width = 40, picture_height = 20;
lv_draw_buf_t* make_picture() {
  lv_draw_buf_t* b = lv_draw_buf_create(picture_width, picture_height, LV_COLOR_FORMAT_ARGB8888, 0);
  if (b == nullptr) return nullptr;
  for (std::uint32_t y = 0; y < b->header.h; ++y) {
    std::uint8_t* row = b->data + y * b->header.stride;
    for (std::uint32_t x = 0; x < b->header.w; ++x) {
      const bool left = x < b->header.w / 2;
      row[4 * x + 0] = left ? 0x00 : 0xff;  // blue
      row[4 * x + 1] = 0x00;                // green
      row[4 * x + 2] = left ? 0xff : 0x00;  // red
      row[4 * x + 3] = 0xff;                // opaque
    }
  }
  return b;
}
lv_draw_buf_t* picture = nullptr;

// What a widget needs before an angle shows. Reflection finds the setters; what
// makes each widget draw something that turns is written here.
void prepare(string_view create, lv_obj_t* o) {
  if (create == "lv_image_create") {
    lv_image_set_src(o, picture);
  } else {
    lv_obj_set_size(o, 150, 150);
  }
  if (create == "lv_scale_create") {
    lv_scale_set_mode(o, LV_SCALE_MODE_ROUND_INNER);
    // Not the default of 11 ticks: with ten gaps, ten turns more of range move every
    // tick by a whole turn, and a range would look like an angle in tenths.
    lv_scale_set_total_tick_count(o, 8);
  }
  if (create == "lv_arclabel_create") {
    lv_arclabel_set_text_static(o, "tenths or degrees, tenths or degrees, tenths or degrees");
    // LVGL v9.6 divides by the radius when it draws, and the radius starts at zero.
    lv_arclabel_set_radius(o, 60);
  }
  lv_obj_center(o);
}

// A widget for one target: of the setter's kind, or a plain box for a style value.
template <target T>
lv_obj_t* make_widget(lv_obj_t* screen) {
  if constexpr (T.create == std::meta::info{}) {
    lv_obj_t* box = lv_obj_create(screen);
    lv_obj_set_size(box, 80, 30);
    lv_obj_center(box);
    return box;
  } else {
    lv_obj_t* o = [:T.create:](screen);
    prepare(std::meta::identifier_of(T.create), o);
    return o;
  }
}

// What the other numbers of a setter with several are, while one is tried.
constexpr std::int32_t resting = 40;

// The type of the value parameter number I of a setter (after the widget).
template <std::meta::info Setter, std::size_t I>
using value_parameter =
    typename[:std::meta::remove_cv(std::meta::type_of(std::meta::parameters_of(Setter)[I + 1])):];

// Calls the setter with `v` as the angle that is tried.
template <target T, std::size_t... I>
void set_angle(lv_obj_t* o, std::int32_t v, std::index_sequence<I...>) {
  if constexpr (T.create == std::meta::info{})
    [:T.setter:](o, v, LV_PART_MAIN);
  else
    [:T.setter:](o, static_cast<value_parameter<T.setter, I>>(I + 1 == T.parameter ? v : resting)...);
}

enum class unit { degrees, tenths, no_visible_effect, not_periodic };

// The pictures of one widget with one of its angles at different values.
struct pictures {
  std::vector<std::uint8_t> at_angle, plus_100, plus_360, plus_3600, at_zero;
};

constexpr const char* text_of(unit u) {
  switch (u) {
    case unit::degrees: return "degrees";
    case unit::tenths: return "tenths of a degree";
    case unit::no_visible_effect: return "no visible effect";
    case unit::not_periodic: return "not periodic";
  }
  return "?";
}

// The unit, from the pictures: at an angle, one and ten turns of 360 units further,
// at zero, and 100 units further. The last two tell an angle that turns something
// from one that changes nothing, or that only switches the drawing on and off.
unit judge(const pictures& p) {
  if (p.at_angle.empty() || p.at_angle == p.at_zero || p.at_angle == p.plus_100)
    return unit::no_visible_effect;
  if (p.plus_360 == p.at_angle) return unit::degrees;
  return p.plus_3600 == p.at_angle ? unit::tenths : unit::not_periodic;
}
static_assert(text_of(unit::degrees) != text_of(unit::tenths));

// The unit of one target, by what LVGL draws.
template <target T>
unit seen(lv_obj_t* screen) {
  constexpr std::size_t values = std::meta::parameters_of(T.setter).size() - 1;
  constexpr std::int32_t start = 70;
  lv_obj_t* o = make_widget<T>(screen);
  const auto set = [&](std::int32_t v) {
    set_angle<T>(o, v, std::make_index_sequence<values>{});
    return shot();
  };
  pictures p;
  p.at_angle = set(start);
  p.plus_100 = set(start + 100);
  p.plus_360 = set(start + 360);
  p.plus_3600 = set(start + 3600);
  p.at_zero = set(0);
  lv_obj_delete(o);
  return judge(p);
}

// The setters whose unit the pixels cannot show, sorted by name.
//  - transform_skew_x and _y: the software renderer of this configuration does not
//    draw a skewed layer ("draw task failed"). Every skew but zero gives the same
//    picture, with the widget missing.
//  - a scale's angle_range is a span, not a direction: a longer span is another
//    picture, and it never comes round.
//  - an arc label's angle_size is a span too: how much of the circle the text may
//    take.
constexpr string_view not_verifiable[] = {
    "lv_arclabel_set_angle_size(size)",
    "lv_obj_set_style_transform_skew_x(value)",
    "lv_obj_set_style_transform_skew_y(value)",
    "lv_scale_set_angle_range(angle_range)",
};

// What was found, for all targets.
struct findings {
  std::vector<string_view> unseen;
  int verified = 0;
};

// Tries target number I: the pixels must say what the comment says, if they say
// anything.
template <std::size_t I>
void try_target(lv_obj_t* screen, findings& found) {
  constexpr target t = targets[I];
  constexpr bool says_tenths = d::angle_type_of(t.setter) == ^^lv::decidegrees;
  const unit said = says_tenths ? unit::tenths : unit::degrees;
  const unit saw = seen<t>(screen);
  std::printf("%-46s comment: %-18s pixels: %s\n", name_of<t>, text_of(said), text_of(saw));
  if (saw == unit::degrees || saw == unit::tenths) {
    check(saw == said);
    ++found.verified;
  } else {
    found.unseen.push_back(name_of<t>);
  }
}

template <std::size_t... I>
findings try_targets(lv_obj_t* screen, std::index_sequence<I...>) {
  findings found;
  (try_target<I>(screen, found), ...);
  return found;
}

void pixels_agree_with_the_comments(lv_obj_t* screen) {
  auto [unseen, verified] = try_targets(screen, std::make_index_sequence<targets.size()>{});
  std::ranges::sort(unseen);
  std::printf("verified %d of %zu angles; not verifiable: %zu\n", verified, targets.size(),
              unseen.size());
  // A setter that cannot be verified is known and explained above, or the test fails.
  check(std::ranges::equal(unseen, not_verifiable));
  // The four that started this: an image's rotation and a style's are in tenths,
  // an arc's and a scale's in degrees. They must be among the verified ones.
  for (string_view must : {"lv_image_set_rotation(angle)", "lv_arc_set_rotation(rotation)",
                           "lv_scale_set_rotation(rotation)",
                           "lv_obj_set_style_transform_rotation(value)"})
    check(std::ranges::find(unseen, must) == unseen.end());
}

// How the unit is judged, on pictures of one byte: at the angle, 100 further, 360
// further, 3600 further, and at zero.
void judges() {
  const std::vector<std::uint8_t> a{1}, b{2}, c{3}, none;
  check(judge({a, b, a, a, c}) == unit::degrees);
  check(judge({a, b, a, c, c}) == unit::degrees);
  check(judge({a, b, c, a, c}) == unit::tenths);
  check(judge({a, b, c, c, c}) == unit::not_periodic);
  check(judge({a, b, a, a, a}) == unit::no_visible_effect);  // zero looks the same
  check(judge({a, a, a, a, c}) == unit::no_visible_effect);  // every angle looks the same
  check(judge({none, b, none, none, c}) == unit::no_visible_effect);  // no picture at all
}

// ---- a quarter turn is a quarter turn ----------------------------------------------

struct rgb {
  int red, green, blue;
  bool operator==(const rgb&) const = default;
};
constexpr rgb red{0xff, 0, 0}, blue{0, 0, 0xff};

// The pixel at (x, y) of a picture of the 320 x 240 screen.
rgb pixel(const std::vector<std::uint8_t>& shot_of_screen, int x, int y) {
  const auto at = static_cast<std::size_t>((y * 320 + x) * 4);
  if (at + 2 >= shot_of_screen.size()) return {-1, -1, -1};
  return {shot_of_screen[at + 2], shot_of_screen[at + 1], shot_of_screen[at]};
}

void image_turns_a_quarter(lv::obj screen) {
  constexpr int cx = 160, cy = 120;  // the middle of the screen, and of the picture

  // Not turned: red on the left, blue on the right.
  auto image = screen.add<"image">().set({.align = LV_ALIGN_CENTER, .src = picture});
  const auto flat = shot();
  check(pixel(flat, cx - 10, cy) == red && pixel(flat, cx + 10, cy) == blue);
  check(image.get<"rotation">() == 0 * deg);

  // Turned by 90 degrees, clockwise: red on top, blue below, nothing at the sides.
  image.set({.rotation = 90 * deg});
  const auto turned = shot();
  check(pixel(turned, cx, cy - 10) == red && pixel(turned, cx, cy + 10) == blue);
  check(pixel(turned, cx - 15, cy) != red && pixel(turned, cx + 15, cy) != blue);
  check(lv_image_get_rotation(image.raw()) == 900);
  check(image.get<"rotation">() == 90 * deg && image.get<"rotation">().tenths == 900);
  static_assert(std::same_as<decltype(image.get<"rotation">()), lv::decidegrees>);

  // The same pixels as LVGL's own call with 900 tenths.
  lv_image_set_rotation(image.raw(), 0);
  check(shot() == flat);
  lv_image_set_rotation(image.raw(), 900);
  check(shot() == turned);

  // Half of that is 45 degrees, which only tenths can say exactly as 22.5 twice.
  image.set({.rotation = 22.5 * deg});
  check(lv_image_get_rotation(image.raw()) == 225);
  check(image.get<"rotation">() == 22.5 * deg && image.get<"rotation">() != 22 * deg);
  lv_obj_delete(image.raw());
}

// An arc is in whole degrees, and says so.
void arc_stays_in_degrees(lv::obj screen) {
  auto arc = screen.add<"arc">().set({.rotation = 90 * deg});
  check(lv_arc_get_rotation(arc.raw()) == 90);
  check(arc.get<"rotation">() == 90 * deg && arc.get<"rotation">().degrees == 90);
  static_assert(std::same_as<decltype(arc.get<"rotation">()), lv::angle>);
  // A getter that is not an angle returns what LVGL returns.
  static_assert(std::same_as<decltype(arc.get<"value">()), std::int32_t>);
  lv_obj_delete(arc.raw());
}

// A style's rotation: set at run time, and from a constant style in a tree.
constexpr auto tilted = lv::box({.id = "tilted", .size = {80 * px, 30 * px},
                                 .style = {.transform_rotation = lv::raw(22.5 * deg),
                                           .transform_skew_x = lv::raw(-3 * deg)}});

void style_rotation_is_in_tenths(lv::obj screen) {
  auto box = screen.add<"obj">().set({.style = {.transform_rotation = lv::raw(90 * deg)}});
  check(lv_obj_get_style_transform_rotation(box.raw(), LV_PART_MAIN) == 900);
  check(box.get<"style_transform_rotation">(LV_PART_MAIN) == 90 * deg);
  static_assert(std::same_as<decltype(box.get<"style_transform_rotation">(LV_PART_MAIN)),
                             lv::decidegrees>);
  lv_obj_delete(box.raw());

  auto root = lv::mount<tilted>(screen);
  check(root.has_value());
  if (!root) return;
  const auto t = root->get<"tilted">();
  check(lv_obj_get_style_transform_rotation(t.raw(), LV_PART_MAIN) == 225);
  check(t.get<"style_transform_skew_x">(LV_PART_MAIN) == -3 * deg);
  check(t.get<"style_transform_skew_y">(LV_PART_MAIN) == 0 * deg);
}

}  // namespace

int main() {
  lv_init();
  lv_test_display_create(320, 240);
  picture = make_picture();
  check(picture != nullptr);
  if (picture != nullptr) {
    auto screen = lv::screen_active();
    judges();
    pixels_agree_with_the_comments(screen.raw());
    image_turns_a_quarter(screen);
    arc_stays_in_degrees(screen);
    style_rotation_is_in_tenths(screen);
    lv_draw_buf_destroy(picture);
  }
  lv_deinit();
  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
