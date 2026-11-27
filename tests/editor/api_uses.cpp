// Every public name of lvgl26, used once. Two compilers read this file: GCC with the
// real headers, as part of the build, and Clang with the editor stubs of
// tools/editor/stubs (the test editor_api_uses). A name that the stubs lack, or
// declare with another shape, fails the second one.
//
// When you add a public name to include/lv, use it here and declare it in the stubs.
// tools/editor/inventory.py fails until both are done.

#include <lv/encoded_image.hpp>
#include <lv/font.hpp>
#include <lv/gesture.hpp>
#include <lv/icons.hpp>
#include <lv/host.hpp>
#include <lv/image.hpp>
#include <lv/list.hpp>
#include <lv/look.hpp>
#include <lv/lv.hpp>
#include <lv/mcp.hpp>
#include <lv/one_of.hpp>
#include <lv/png.hpp>
#include <lv/pointer.hpp>
#include <lv/repeat.hpp>
#include <lv/runtime.hpp>
#include <lv/scaled_font.hpp>
#include <lv/shown.hpp>
#include <lv/signal.hpp>
#include <lv/signal_lamps.hpp>
#include <lv/signal_gesture.hpp>
#include <lv/signal_motion.hpp>
#include <lv/signal_type.hpp>
#include <lv/theme.hpp>
#include <lv/ui_scheduler.hpp>

#include <mp-units/systems/si.h>

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <expected>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace api_uses {

using namespace lv::units;
using mp_units::si::unit_symbols::h;
using mp_units::si::unit_symbols::km;
using mp_units::si::unit_symbols::min;

// ---- settings ---------------------------------------------------------------------

constexpr lv::config settings = lv::cfg<lv::override>;
static_assert(settings.allow.allocation && settings.allow.threads && settings.allow.files &&
              settings.allow.floating_point);
static_assert(!settings.rules.raw_style_values && !settings.rules.raw_integer_state);
static_assert(settings.limits.compile_time_image.value == 128 * 128);
static_assert(lv::settings_file<lv::override>.size == 0);
constexpr lv::allow_settings no_threads{.threads = false};
constexpr lv::rule_settings plain_styles{.raw_style_values = true};
constexpr lv::limit_settings small_pictures{.compile_time_image = lv::pixel_count{64 * 64}};
constexpr lv::byte_count stack{4096};
constexpr lv::settings_unit kib = lv::byte_count::units[1];
static_assert(!no_threads.threads && plain_styles.raw_style_values && stack.value == 4096 && kib.factor == 1024);
static_assert(small_pictures.compile_time_image.value == 64 * 64);
// Clang reads no annotation, so the description of a setting is used as a value here.
constexpr lv::described setting_text{"A setting of a project."};
static_assert(setting_text.text[0] == 'A');
constexpr char settings_doc[] = "{}";
constexpr lv::settings_text settings_doc_text{settings_doc, sizeof settings_doc - 1, "lvgl26.json"};
static_assert(settings_doc_text.size == 2);

// ---- units ------------------------------------------------------------------------

constexpr lv::length wide = 320 * px;
constexpr lv::percent half = 50 * pct;
constexpr lv::length half_wide = half;
constexpr lv::angle quarter = 90 * deg;
constexpr lv::decidegrees fine = 22.5 * deg;
constexpr lv::decidegrees also_fine = quarter;
constexpr lv::duration soon = 300 * ms;
constexpr lv::duration later = 2 * sec;
constexpr lv::opa faint = 50 * pct;
constexpr lv::horizontal across = wide;
constexpr lv::vertical down = lv::size_content;
constexpr lv::along<lv::x_axis> also_across = across;
constexpr lv::along<lv::y_axis> also_down = down;
constexpr auto blue = lv::rgb(0x3366ff);
static_assert(wide.raw() == 320 && half.value == 50 && quarter.degrees == 90 && fine.tenths == 225);
static_assert(soon.ms == 300 && later.raw() == 2000 && fine == lv::decidegrees::from_number(22.5));
static_assert(lv::opa::cover.raw() == 255 && lv::opa::transparent.raw() == 0 && faint == lv::opa::from_raw(127));
static_assert(lv::length::px(3) == 3 * px && lv::length::content() == lv::size_content && also_fine == quarter);
static_assert(across.get() == wide && also_across.raw() == 320 && down == also_down && half_wide.raw() != 0);
static_assert(blue.blue == 0xff);
static_assert(std::same_as<decltype(lv::px), const lv::px_unit> && std::same_as<decltype(lv::pct), const lv::pct_unit>);
static_assert(std::same_as<decltype(lv::deg), const lv::deg_unit> && std::same_as<decltype(lv::ms), const lv::ms_unit>);
static_assert(std::same_as<decltype(lv::sec), const lv::sec_unit>);

constexpr lv::counter three = [] {
  lv::counter c{};
  ++c, ++c, --c;
  c += 3;
  c -= 1;
  return c;
}();
static_assert(three.value == 3 && three > lv::counter{2});

// ---- tokens and their types -------------------------------------------------------

constexpr char tokens_json[] = {
#embed "tokens.json"
    , 0};
constexpr auto tok = lv::tokens<tokens_json>;

static_assert(std::same_as<decltype(tok.color.ink), lv::token<lv_color_t>>);  // C API
static_assert(std::same_as<decltype(tok.color.accent), lv::token<lv_color_t>>);  // C API
static_assert(std::same_as<decltype(tok.space.gap), lv::token<lv::length>>);
static_assert(std::same_as<decltype(tok.space._2xl), lv::token<lv::length>>);
static_assert(std::same_as<decltype(tok.opacity.half), lv::token<double>>);
static_assert(std::same_as<decltype(tok.time.quick), lv::token<lv::duration>>);
static_assert(std::same_as<decltype(tok.motion.fall), lv::token<lv::bezier>>);
static_assert(std::same_as<decltype(tok.font.text), lv::token<std::string_view>>);
static_assert(std::same_as<decltype(tok.text.label), lv::token<lv::typography_token>>);
static_assert(std::same_as<decltype(tok.card.border), lv::token<lv::border_token>>);
static_assert(std::same_as<decltype(tok.card.shadow), lv::token<lv::shadow_token>>);
static_assert(std::same_as<decltype(tok.card.fade), lv::token<lv::transition_token>>);

constexpr lv::bezier line{};
constexpr lv::shadow_token no_shadow{};
constexpr lv::border_token no_border{};
constexpr lv::typography_token no_type{};
constexpr lv::transition_token no_transition{};
constexpr lv::gradient_token no_gradient{};
constexpr lv::gradient_stop no_stop{};
constexpr lv::rgba_color no_alpha{};
static_assert(no_alpha.alpha == 255);
static_assert(line.x2 == 1 && no_shadow.blur == lv::length{} && no_border.width == lv::length{});
static_assert(no_type.font_weight == 400 && no_transition.delay == lv::duration{} && no_gradient.count == 0);
static_assert(no_stop.position == 0);

constexpr lv::length gap = tok.space.gap;
constexpr auto raw_gap = lv::raw(3 * px);
static_assert(std::same_as<decltype(raw_gap), const lv::raw_value<lv::length>>);
static_assert(gap == *tok.space.gap && raw_gap.value == 3 * px);

// A stub token holds the value of a length, a time and a number, so code that needs a token's
// value while it compiles, such as the size of a frame, reads in the editor as it builds.
static_assert(tok.space.gap->raw() == 8 && tok.space._2xl->raw() == 32);
static_assert(tok.time.quick->raw() == 150 && *tok.opacity.half == 0.5);
static_assert(lv::frame<tok.space._2xl->raw(), tok.space.gap->raw()>::width == 32);
static_assert(std::same_as<decltype(lv::signal::dark_tokens.color.muted), lv::token<lv_color_t>>);  // C API
static_assert(std::same_as<decltype(lv::signal::day_tokens.color.ground), lv::token<lv_color_t>>);  // C API

// ---- Signal's typefaces -----------------------------------------------------------

constexpr auto signal_title = lv::signal::type(lv::signal::type_tokens.text.title);
constexpr auto signal_reading = lv::signal::numerals(lv::signal::type_tokens.text.numeral_l);
constexpr lv::typeface own_barlow{{"Barlow", 500, lv::signal::barlow_medium_ttf},
                                  {"Barlow Condensed", 600, lv::signal::barlow_condensed_semibold_ttf},
                                  {"Barlow Condensed", 700, lv::signal::barlow_condensed_bold_ttf}};
constexpr auto signal_fonts = std::pair{lv::signal::body_font, lv::signal::label_font};
constexpr lv::length signal_spacing = lv::signal::label_spacing;
static_assert(std::same_as<decltype(lv::signal::lamps.dark.green), lv::token<lv_color_t>>);  // C API
static_assert(std::same_as<decltype(lv::signal::lamps.day.blue), lv::token<lv_color_t>>);  // C API
static_assert(std::same_as<decltype(lv::signal::motion.duration.settle), lv::token<lv::duration>>);
static_assert(std::same_as<decltype(lv::signal::motion.ease.back), lv::token<lv::bezier>>);

// ---- styles and themes ------------------------------------------------------------

constexpr auto card = lv::style({.bg_color = tok.color.ink,
                                 .bg_grad_dir = "ver",
                                 .bg_opa = tok.opacity.half,
                                 .pad_all = tok.space.gap,
                                 .radius = lv::raw(3 * px)}) |
                      tok.card.border | tok.card.shadow | tok.card.fade | tok.text.label;
constexpr auto loud = card | lv::style({.bg_color = tok.color.accent});
constexpr auto knob = loud | lv::on("knob", "pressed", {.bg_color = tok.color.ink}) |
                      lv::on("checked", {.bg_opa = tok.opacity.half});

constexpr lv::theme_styles look = lv::theme({
    .button = {.style = card, .style_pressed = {.bg_color = tok.color.accent}},
    .obj = {.style = {.pad_gap = tok.space.gap}},
    .screen = {.style = {.bg_color = tok.color.ink}},
    .slider = {.style = knob},
    .toggle = {.style_indicator = {.bg_opa = tok.opacity.half}},
});
static_assert(!look.empty() && lv::theme_styles{}.empty());

// ---- fonts and pictures -----------------------------------------------------------

constexpr unsigned char barlow_ttf[] = {
#embed "fonts/Barlow-Medium.ttf"
};
constexpr unsigned char picture_png[] = {
#embed "dot.png"
};

constexpr auto type = lv::typeface({{"Barlow", 500, barlow_ttf, "0123456789"}});
constexpr auto label_font = type(tok.text.label);
constexpr auto odd_font = lv::font<barlow_ttf>(12 * px, "0123456789");
constexpr auto priced_font = lv::font<barlow_ttf>(12 * px, lv::ascii_plus<"€·">);
constexpr lv::face_file face{"Barlow", 500, barlow_ttf};
static_assert(std::same_as<decltype(label_font), const lv::token<const lv_font_t*>>);  // C API
static_assert(std::same_as<decltype(odd_font), const lv_font_t* const>);  // C API
static_assert(std::same_as<decltype(priced_font), const lv_font_t* const>);  // C API

// lv::like and lv::name_of in a property: the editor reads each as a value of the property's type.
enum class roast { light, dark };
struct roast_names {
  static constexpr char light[] = "Light";
  static constexpr char dark[]  = "Dark";
};
struct roast_pictures {
  static constexpr auto light = lv::image_file<picture_png>;
  static constexpr auto dark  = lv::image_file<picture_png>;
};
template <roast R>
constexpr auto roast_tile() {
  return lv::button(lv::image({.src = lv::like<R, roast_pictures>}),
                    lv::label({.text = lv::like<R, roast_names>}), lv::label({.text = lv::name_of<R>}));
}
constexpr auto roast_tiles = lv::row(roast_tile<roast::light>(), roast_tile<roast::dark>());
static_assert(face.weight == 500 && face.family == "Barlow" && face.characters.empty());

constinit lv::scaled_font reading{barlow_ttf, tok.text.label};
constinit lv::scaled_font other_reading{barlow_ttf, lv::raw(20 * px)};

constexpr auto gear_text = lv::glyph<"gear">;
constexpr auto icon_list = lv::glyphs<"gear", "plus">;
constexpr auto gear_icon = lv::icon<"gear">({.id = "gear"});
static_assert(std::same_as<decltype(gear_text), const char* const>);

constexpr auto dot = lv::image_file<picture_png>;
constexpr auto kept_dot = lv::encoded_image<picture_png>;
constexpr auto dot_mask = lv::mask_file<picture_png>;
static_assert(std::same_as<decltype(dot), const lv_image_dsc_t* const>);  // C API
static_assert(std::same_as<decltype(kept_dot), const lv_image_dsc_t* const>);  // C API
static_assert(std::same_as<decltype(dot_mask), const lv_image_dsc_t* const>);  // C API

// ---- state ------------------------------------------------------------------------

using speed_t = decltype(0 * km / h);
enum class mode { off, eco, sport };
struct tab_index {
  int value;
};

constinit lv::state speed{0 * km / h};
constinit lv::state limit{50 * km / h};
constinit lv::state clicks{lv::counter{}};
constinit lv::state<mode> current_mode{mode::off};
constinit lv::state shown{true};
constinit lv::state tab{tab_index{0}};
constinit lv::computed over_limit{[](speed_t s, speed_t l) { return s > l; }, speed, limit};
constinit lv::computed gap_to_limit{[](speed_t s, speed_t l) { return l - s; }, speed, limit};
constinit lv::effect log{[](speed_t) {}, speed};

static_assert(std::same_as<decltype(over_limit)::value_type, bool>);
static_assert(std::same_as<decltype(speed.get()), speed_t>);
static_assert(std::same_as<decltype(lv::bind(speed)), lv::binding<speed_t>>);
static_assert(std::same_as<decltype(lv::bind(clicks, "{} clicks")), lv::formatted_binding<lv::counter>>);
static_assert(std::same_as<decltype(lv::bind(over_limit)), lv::binding<bool>>);
static_assert(std::same_as<decltype(lv::bind(gap_to_limit, "{} left")), lv::formatted_binding<speed_t>>);
static_assert(std::same_as<decltype(lv::bind(reading)), lv::font_binding>);

void reset() { speed = 0 * km / h; }
void seen(lv::event e) { static_cast<void>(e.code()), static_cast<void>(e.raw()), static_cast<void>(e.target<lv::obj>()); }

void use_state() {
  speed = 120 * km / h;
  speed.set(2 * km / min);
  speed += 10 * km / h;
  speed -= 10 * km / h;
  ++clicks;
  --clicks;
  clicks.update([](lv::counter c) { return ++c; });
  static_cast<void>(speed.number());
  static_cast<void>(clicks.number());
  static_cast<void>(tab.get().value);
  static_cast<void>(shown.raw());
  static_cast<void>(shown.observe([](bool) {}));
  shown.reset();
  static_cast<void>(over_limit.get());
  static_cast<void>(over_limit.raw());
  over_limit.reset();
  static_cast<void>(log.start());
  log.reset();
  reading.resize(tok.text.label);
  reading.resize(lv::raw(31 * px));
  static_cast<void>(reading.size());
  static_cast<void>(reading.raw());
  reading.reset();
  other_reading.reset();
}

// ---- option lists -----------------------------------------------------------------

enum class quality { best, medium, draft };
constinit lv::state<quality> chosen_quality{quality::medium};
static_assert(std::same_as<decltype(lv::options_of<quality>), const lv::option_list>);
static_assert(std::same_as<decltype(lv::options_of<quality, lv::but<quality::draft>>), const lv::option_list>);
static_assert(std::is_empty_v<lv::but<quality::draft>>);
static_assert(std::convertible_to<const char*, lv::option_list>);
static_assert(requires { lv::option_text<quality>::of(quality::best); });

// ---- the tree ---------------------------------------------------------------------

constexpr auto ui = lv::column({.flex_align = {"center", "center", "start"}, .size = {320 * px, 240 * px}})[
    lv::label({.id = "title", .text = "Speed", .style = {.text_font = label_font}}),
    lv::label({.text = lv::bind(speed), .style = {.text_font = lv::raw(odd_font)}}),
    lv::label({.text = lv::bind(clicks, "{} clicks"), .style = {.text_font = lv::bind(reading)}}),
    lv::arc({.id = "speedo", .range = {0, 240}, .rotation = 90 * deg, .value = lv::bind(speed)}),
    lv::bar({.id = "level", .value = {10, false}, .width = 50 * pct}),
    lv::slider({.id = "volume", .value = {3, false}, .style_knob = card}),
    lv::toggle({.id = "power", .checked = true}),
    lv::checkbox({.id = "agree", .text = "Yes"}),
    lv::image({.id = "picture", .align = "center", .rotation = 22.5 * deg, .src = dot}),
    lv::image({.src = kept_dot}),
    lv::textarea({.id = "note", .one_line = true}),
    lv::dropdown({.id = "choice", .options = "a\nb"}),
    lv::dropdown({.id = "sizes", .options = {"100%", "75%", "50%"}}),
    lv::dropdown({.id = "grade", .options = lv::options_of<quality, lv::but<quality::draft>>,
                  .selected = lv::bind(chosen_quality)}),
    lv::roller({.id = "wheel", .options = {{"Mon", "Tue", "Wed"}, "infinite"}}),
    lv::spinner({.id = "busy"}),
    lv::widget<"spinbox">({.id = "count", .range = {0, 9}}),
    lv::row[lv::box({.id = "card", .hidden = lv::bind(over_limit), .style = loud})],
    lv::group[lv::button({.id = "reset", .style_pressed = {.bg_color = tok.color.ink}, .on_clicked = &reset, .on_pressed = &seen})[
        lv::label({.text = "Reset"})]]];

static_assert(ui.id().empty());
static_assert(std::same_as<lv::obj, lv::view<"obj">>);
static_assert(lv::view<"arc">::kind == "arc");

constexpr lv::props<"arc"> arc_props{.value = 1};
constexpr lv::animatable<"arc"> arc_ends{.value = {0, 100}};
constexpr lv::from_to<lv::horizontal> widths{80 * px, 120 * px};
static_assert(widths.is_set() && widths.from() == lv::horizontal{80 * px} && widths.to() == lv::horizontal{120 * px});

// ---- what a props struct is made of -----------------------------------------------

constexpr lv::arg<lv::angle> turn{90 * deg};
constexpr lv::strict<std::int32_t> whole{3};
constexpr lv::strict<bool> truth{true};
constexpr lv::nonnegative<lv::angle> sweep{270 * deg};
constexpr lv::handler on_reset{&reset};
constexpr lv::styled<lv::length> from_token{tok.space.gap};
constexpr lv::styled<lv::length> not_from_token{lv::raw(3 * px)};
constexpr lv::text_format in_kmh{"{} km/h"};
static_assert(whole.raw() == 3 && truth.raw());
static_assert(sweep.get() == 270 * deg);
static_assert(std::same_as<decltype(lv::label()), lv::node<"label">>);
static_assert(std::same_as<decltype(lv::view<"button">{}.next().clicked), lv::event_ref<LV_EVENT_CLICKED>>);  // C API
static_assert(std::same_as<decltype(lv::mount<ui>(lv::obj{})), lv::mounted<ui>>);
static_assert(lv::storage_size<ui> == sizeof(lv::mounted<ui>));

// ---- an application, a display, a mounted tree ------------------------------------

// A window as a platform makes one: it joins the loop, shows the frame, feeds a pointer.
struct editor_window {
  lv::in_loop<editor_window> loop;
  explicit editor_window(lv::application& app) : loop{app, *this} {}
  void poll() {}
  void wait(lv::duration) {}
  void operator()(lv::area) {}
  void attach(lv::display&) {}
  void detach(lv::display&) {}
  void attach(lv::pointer_feed) {}
  void detach(lv::pointer_feed) {}
  void hide() {}
  void show() {}
};

constinit lv::mounted<ui> page;
constexpr auto popup = lv::column[lv::label({.text = "Popup"})];
constinit lv::one_of<ui, popup> slot;

// lv::shown and lv::when: a page for the value of a state, a tree while a flag is true.
enum class screen_page { main, popup_page, none };
constinit lv::state<screen_page> current_page{screen_page::main};
constinit lv::state<bool> busy{false};
void on_popup(lv::held<popup> shown_popup) { static_cast<void>(shown_popup.has_value()); }
void popup_in_front(lv::held<popup> front_popup) { static_cast<void>(front_popup.has_value()); }
constexpr auto pager = lv::group[lv::shown(current_page)[
    lv::is<screen_page::main>(ui), lv::is<screen_page::popup_page>(popup, {.on_shown = &on_popup, .on_front = &popup_in_front}), lv::otherwise()]];
constexpr auto spinner_while_busy = lv::group[lv::when(busy)[popup]];
constexpr auto empty_arm = lv::is<screen_page::none>();
constinit const lv::page popup_page = lv::page_of<popup, {.on_shown = &on_popup, .on_front = &popup_in_front}>;
constinit const lv::page main_page = lv::page_of<ui>;
constexpr auto slide_in = lv::transition({.x = {320 * px, 0 * px}}, 200 * lv::ms);
constexpr auto slide_out = lv::transition({.x = {0 * px, 320 * px}}, 200 * lv::ms);
constexpr auto back_in = lv::transition({.x = {-320 * px, 0 * px}}, 200 * lv::ms);
constexpr auto back_out = lv::transition({.x = {0 * px, -320 * px}}, 200 * lv::ms);
constexpr auto two_ways_pages =
    lv::group[lv::shown(current_page, {.enter = slide_in, .leave = slide_out, .enter_back = back_in, .leave_back = back_out})[
        lv::is<screen_page::main>(main_page),
        lv::is<screen_page::popup_page>(popup_page, {.enter = back_in, .leave = back_out}), lv::otherwise()]];
constexpr auto sliding_pages = lv::group[lv::shown(current_page, {.alive = 2, .enter = slide_in, .leave = slide_out})[
    lv::is<screen_page::main>(main_page), lv::is<screen_page::popup_page>(popup_page), lv::otherwise()]];
static_assert(lv::page_of<popup>.size > 0);

// The same with the children as arguments: properties first, then each child.
constexpr auto pager_call = lv::group(lv::shown(
    current_page, lv::is<screen_page::main>(ui), lv::is<screen_page::popup_page>(popup), lv::otherwise()));
constexpr auto sliding_call = lv::group(lv::shown(
    current_page, {.alive = 2, .enter = slide_in, .leave = slide_out}, lv::is<screen_page::main>(ui),
    lv::otherwise()));
constexpr auto spinner_call = lv::group({.id = "spin"}, lv::when(busy, popup));
static_assert(std::same_as<decltype(lv::when(busy, popup)), decltype(lv::when(busy)[popup])>);
static_assert(std::same_as<decltype(lv::shown(current_page, lv::is<screen_page::main>(ui), lv::otherwise())),
                           decltype(lv::shown(current_page)[lv::is<screen_page::main>(ui), lv::otherwise()])>);
constexpr auto boxed = lv::column({.size = {100 * pct, lv::size_content}}, lv::label({.text = "a"}), lv::label());
constexpr auto boxed_bare = lv::row(lv::label({.text = "a"}), lv::button({.id = "go"}, lv::label({.text = "Go"})));
// The call form and the subscript form build the same node.
static_assert(std::same_as<std::remove_const_t<decltype(boxed)>, decltype(lv::column({.size = {100 * pct, lv::size_content}})[
                                              lv::label({.text = "a"}), lv::label()])>);
static_assert(std::same_as<std::remove_const_t<decltype(boxed_bare)>, decltype(lv::row[lv::label({.text = "a"}),
                                                                   lv::button({.id = "go"})[lv::label({.text = "Go"})]])>);

constinit lv::frame<320, 240> frame;
constinit lv::band<320, 240, 10> band;
static_assert(lv::frame<320, 240>::width == 320 && lv::frame<320, 240>::height == 240 && lv::frame<320, 240>::stride > 0);
static_assert(lv::band<320, 240, 10>::lines == 10 && lv::band<320, 240, 10>::width == 320 && lv::band<320, 240, 10>::height == 240);

void changed(lv::area) {}
void send(lv::area, std::span<const std::byte>) {}
struct frame_listener {
  void operator()(lv::area) {}
};
struct band_listener {
  void operator()(lv::area, std::span<const std::byte>) {}
};
static_assert(lv::frame_panel<frame_listener> && lv::band_panel<band_listener>);

lv::duration since_start() { return 0 * ms; }
void wait(lv::duration) {}

int use_application() {
  lv::application app{{.clock = &since_start, .idle = &wait}};
  app.advance(5 * ms);
  const lv::duration next = app.step();
  const lv::heap_use heap = app.heap();
  app.quit(static_cast<int>(next.ms + heap.total + heap.used + heap.largest_free));

  frame_listener on_frame;
  band_listener on_band;
  lv::display plain{app, frame};
  lv::display themed{app, frame, lv::signal::dark};
  lv::display day{app, frame, &changed, lv::signal::day};
  lv::display typed{app, frame, lv::signal::dark_with_type};
  lv::display typed_day{app, frame, &changed, lv::signal::day_with_type};
  lv::display object{app, frame, on_frame, look};
  lv::display banded{app, band, &send};
  lv::display banded_object{app, band, on_band, look};
  static_cast<void>(plain.width() + plain.height());
  static_cast<void>(plain.raw());
  static_cast<void>(frame.bytes());
  static_cast<void>(lv::screen_active());
  const lv::area all{0, 0, 320, 240};
  static_cast<void>(all == lv::area{});

  lv::pointer mouse{plain};
  editor_window window{app};
  lv::display windowed{app, frame, window};
  lv::pointer fed{windowed, window};
  lv::pointer tuned{plain, lv::gesture_limits{.swipe_distance = 30 * px}};
  static_cast<void>(tuned.raw());
  lv::pointer signal_limits{plain, lv::signal::gesture};
  static_cast<void>(signal_limits.raw());
  static_cast<void>(fed.handle());
  const lv::pointer_feed feed = mouse.handle();
  if (!feed || !(feed == feed) || !feed(lv::point{1, 1}, lv::released)) return 21;
  windowed.hide();
  windowed.show();
  windowed.set_visible(windowed.visible());
  static_cast<void>(lv::display_of(windowed.screen()).width());
  static_assert(lv::loop_source<editor_window> && lv::waiting_loop_source<editor_window>);
  static_assert(lv::attached_panel<editor_window> && lv::hideable_panel<editor_window> && lv::pointer_feeder<editor_window>);
  if (!window.loop.joined()) return 22;
  const lv::point at{10, 20};
  if (!mouse.press(at) || !mouse.move({11, 20}) || !mouse.release() || !mouse.feed(at, lv::released)) return 20;
  static_assert(std::same_as<decltype(lv::pointer_sample{at, lv::pressed}.state), lv::contact>);
  lv::pointer<4> small{plain};
  lv::pointer panel{plain, [] { return lv::pointer_sample{{1, 2}, lv::pressed}; }};
  static_cast<void>(small.raw() == panel.raw());
  static_assert(lv::most_pointer_samples >= 16);
  auto root = lv::mount<ui>(plain.screen());
  if (!root) return root.error() == lv::mount_error::out_of_memory ? 1 : 2;

  static_assert(std::same_as<decltype(root->get<"speedo">()), lv::view<"arc">>);
  static_assert(std::same_as<decltype(root->get<"title">()), lv::view<"label">>);
  static_assert(std::same_as<decltype(root->get<"power">()), lv::view<"switch">>);
  static_assert(std::same_as<decltype(root->get<"count">()), lv::view<"spinbox">>);
  static_assert(std::same_as<decltype(root->get<"card">()), lv::view<"obj">>);
  static_assert(std::same_as<decltype(root->get<"reset">()), lv::view<"button">>);
  static_assert(std::same_as<decltype(root->get<"speedo">().get<"value">()), std::int32_t>);
  static_assert(std::same_as<decltype(root->get<"speedo">().get<"rotation">()), lv::angle>);
  static_assert(std::same_as<decltype(root->get<"picture">().get<"rotation">()), lv::decidegrees>);
  static_assert(std::same_as<decltype(root->get<"title">().get<"text">()), char*>);
  static_assert(std::same_as<decltype(root->get<"title">().get<"height">()), std::int32_t>);

  lv::view<"arc"> dial = root->get<"speedo">();
  dial.set({.value = 60}).set<"value">(61);
  const lv::obj any = dial;
  static_cast<void>(any == root->root());
  static_cast<void>(dial.raw());
  if (!dial) return 3;
  lv::view<"label"> made = any.add<"label">();
  static_cast<void>(lv::view<"label">::create(any));
  made.on<LV_EVENT_CLICKED>([] {});  // C API

  lv::weak<lv::view<"label">> watched{made};
  lv::owned<lv::view<"label">> kept{made};
  static_cast<void>(watched.get());
  static_cast<void>(watched.expired());
  static_cast<void>(kept.get());
  static_cast<void>(kept.release());
  kept.reset();

  if (!lv::write_png(plain.screen(), "screen.png")) return 4;
  if (auto written = lv::write_new_png(plain.screen(), "new.png"); !written)
    return written.error() == lv::png_error::file_exists ? 5 : 6;
  const auto bytes = lv::encode_png(plain.screen());
  static_assert(std::same_as<decltype(bytes->size()), std::size_t>);

  static_cast<void>(lv::place<ui>(plain.screen()));
  static_cast<void>(root->detach());

  if (page || page.has_value()) return 7;
  if (page.error() != lv::mount_error::not_mounted) return 8;
  if (!page.mount(plain.screen())) return 9;
  static_cast<void>((*page).root());
  page.unmount();
  if (!page.mount(plain.screen())) return 10;
  alignas(lv::mounted<ui>) std::array<std::byte, lv::storage_size<ui>> memory;
  lv::mounted<ui>& in_memory = lv::mount_in<ui>(memory, plain.screen());
  std::destroy_at(&in_memory);
  lv::mounted<ui> direct{plain.screen()};
  if (!slot.mount<popup>(plain.screen())) return 11;
  if (!slot.has_value() || !slot || !slot.holds<popup>()) return 12;
  const lv::held<popup> handle = slot.get<popup>();
  if (!handle || !handle.has_value() || !handle->root() || !(*handle).root()) return 14;
  const std::optional<int> which = slot.visit([](lv::held<ui>) { return 1; }, [](auto) { return 2; });
  const int or_empty = slot.visit([](auto) { return 1; }, [](lv::not_mounted) { return 0; });
  slot.visit([](lv::held<popup>) {}, [](auto) {});
  static_assert(std::same_as<decltype(slot.visit([](auto) { return 1; })), std::optional<int>>);
  static_assert(std::same_as<decltype(slot.visit([](auto) { return 1; }, [](lv::not_mounted) { return 0; })), int>);
  static_assert(std::same_as<decltype(slot.visit([](auto) {})), void>);
  static_assert(std::same_as<decltype(slot.visit([](lv::held<ui>) { return 1; }, [](auto) { return 2.5; },
                                                 [](lv::not_mounted) { return 0; })),
                             double>);
  static_assert(std::same_as<decltype(slot.visit<long>([](auto) { return 1; })), std::optional<long>>);
  slot.visit<void>([](auto) { return 1; });
  slot.unmount();
  auto paged = lv::mount<pager>(plain.screen());
  auto waiting = lv::mount<spinner_while_busy>(plain.screen());
  current_page = screen_page::popup_page;
  busy = true;
  lv::settle();
  if (!paged || !waiting) return 15;
  auto slid = lv::mount<sliding_pages>(plain.screen());
  if (!slid) return 16;
  if (slot.error() != lv::mount_error::not_mounted || which.value_or(0) + or_empty < 0) return 13;
  static_cast<void>(direct.root());
  lv::application::options none{};
  static_cast<void>(none.clock == lv::host.clock);
  return app.run();
}

// ---- lists ------------------------------------------------------------------------

struct track_id {
  int value;
};
struct track {
  track_id id;
  const char* title;
  lv::counter plays;
};

constinit lv::list<track, 8> tracks;
constinit lv::state picked{track_id{0}};
void row_built(lv::obj, const track&) {}

constexpr auto track_row = lv::row[lv::label({.id = "title", .text = lv::bind(&track::title)}),
                                   lv::label({.text = lv::bind(&track::plays, "{} plays")})];
constexpr auto playlist = lv::column[
    lv::each(tracks, &track::id, {.on_row = &row_built, .selected = lv::bind(picked)})[track_row]];
constexpr auto playlist_call = lv::column(
    {.size = {100 * pct, lv::size_content}},
    lv::each(tracks, &track::id, {.on_row = &row_built, .selected = lv::bind(picked)}, track_row));
constexpr auto playlist_plain = lv::column(lv::each(tracks, &track::id, track_row));
// A row written in the call: a temporary, which the deleted fallback of lv::each would bind better
// than a const reference.
constexpr auto playlist_inline = lv::column(lv::each(tracks, &track::id, lv::label({.text = lv::bind(&track::title)})));
static_assert(std::same_as<std::remove_const_t<decltype(playlist_inline)>,
                           decltype(lv::column[lv::each(tracks, &track::id)[lv::label({.text = lv::bind(&track::title)})]])>);
static_assert(std::same_as<std::remove_const_t<decltype(playlist_plain)>,
                           decltype(lv::column[lv::each(tracks, &track::id)[track_row]])>);

static_assert(std::same_as<decltype(lv::bind(&track::title)), lv::detail::row_binding<const char*>>);
static_assert(std::same_as<decltype(lv::bind(&track::plays, "{} plays")), lv::detail::row_formatted_binding<lv::counter>>);
static_assert(lv::list<track, 8>::capacity() == 8);

int use_lists(lv::obj screen) {
  auto root = lv::mount<playlist>(screen);
  if (!root) return 1;
  const std::expected<void, lv::list_error> added = tracks.push_back({{1}, "Intro", {}});
  if (!added) return added.error() == lv::list_error::full ? 2 : 3;
  if (!tracks.insert(0, {{2}, "Outro", {}})) return 4;
  const track more[] = {{{3}, "Bridge", {}}};
  if (!tracks.assign(more)) return 5;
  tracks.update(0, [](track& t) { ++t.plays; });
  tracks.move(0, 0);
  int sum = static_cast<int>(tracks.size()) + (tracks.empty() ? 1 : 0) + (tracks.telling() ? 1 : 0);
  for (const track& t : tracks) sum += t.id.value + tracks[0].plays.value + tracks.count().get().value;
  const lv::row_view<track_row> first{lv::obj{lv_obj_get_child(root->root().raw(), 0)}};  // C API
  static_assert(std::same_as<decltype(first.get<"title">()), lv::view<"label">>);
  static_cast<void>(first.root());
  tracks.erase(0);
  tracks.clear();
  return sum;
}

// ---- tasks ------------------------------------------------------------------------


// What lv::in gives, and the struct that it animates.
template <class T>
inline constexpr bool is_view_in = false;
template <lv::detail::fixed W, lv_style_selector_t S>  // C API
inline constexpr bool is_view_in<lv::view_in<W, S>> = true;
static_assert(is_view_in<decltype(lv::in<"knob", "pressed">(std::declval<lv::view<"arc">>()))>);
static_assert(std::same_as<decltype(lv::animatable_style<>{}.arc_width), lv::from_to<lv::length>>);

static_assert(lv::gesture_settings<lv::gesture_limits>);
lv::state<lv::percent> hold_progress{lv::percent{0}};
void on_swipe(lv::swipe) {}
constexpr auto swipe_page = lv::box({.id = "swipes", .on_swiped = &on_swipe});

lv::task<> count_clicks(lv::view<"button"> button, lv::view<"arc"> arc) {
  const lv::swipe direction = co_await lv::swiped(button);
  static_cast<void>(direction);
  co_await lv::swiped<lv::swipe::left, lv::swipe::right>(button);
  const bool held = co_await lv::hold(button, 1 * sec, hold_progress);
  static_cast<void>(held);
  co_await button.next().clicked;
  co_await button.next().value_changed;
  co_await lv::sleep(100 * ms);
  const speed_t now = co_await lv::changed(speed);
  static_cast<void>(now);
  const bool finished = co_await lv::animate(arc, {.value = {0, 100}}, 300 * ms);
  static_cast<void>(finished);
  co_await lv::animate(arc, {.width = {80 * px, 120 * px}}, 200 * ms, lv::ease::in_quad);
  co_await lv::animate(arc, {.rotation = {0 * deg, 90 * deg}}, 200 * ms, tok.motion.fall);
  co_await lv::animate(arc, {.value = {0, 1}}, 200 * ms, lv::bezier{0.33, 0, 0.67, 0.33});
  // Local style values: of the main part, and of a part and a state behind lv::in.
  co_await lv::animate(arc, {.opa = {0 * pct, 100 * pct}}, 220 * ms);
  co_await lv::animate(arc, {.translate_y = {0 * px, -24 * px}}, 220 * ms, lv::ease::out_quad);
  co_await lv::animate(lv::in<"indicator">(arc), {.arc_width = {2 * px, 8 * px}}, 120 * ms);
  co_await lv::animate(lv::in<"pressed">(arc), {.transform_scale = {256, 280}}, 120 * ms);
  co_await lv::animate(lv::in<"knob", "pressed">(arc), {.opa = {20 * pct, 100 * pct}}, 120 * ms);
  co_await button.next().gone;
}

lv::task<bool> confirm(lv::view<"button"> ok, lv::view<"button"> cancel) {
  const std::size_t which = co_await lv::first(ok.next().clicked, lv::sleep(1 * sec));
  static_cast<void>(which);
  co_return co_await lv::first(ok.next().clicked.then([] { return true; }),
                               cancel.next().clicked.then([] { return false; }),
                               lv::changed(current_mode).then([](mode m) { return m == mode::off; }),
                               lv::sleep(5 * sec).then([] { return false; }));
}

lv::task<> ask(lv::view<"button"> ok, lv::view<"arc"> arc, lv::ui_scheduler& ui_thread) {
  const bool yes = co_await confirm(ok, ok);
  static_cast<void>(yes);
  co_await ui_thread.schedule();
  static_cast<void>(lv::spawn(count_clicks(ok, arc)));
  static_cast<void>(lv::spawn(count_clicks(ok, arc), arc));
}

void use_scheduler() {
  lv::ui_scheduler ui_thread;
  lv::ui_scheduler slow{100 * ms};
  if (ui_thread && slow) static_cast<void>(ui_thread.post([] {}));
}

constexpr lv::easing eases[] = {
    lv::easing{},           lv::ease::linear,      lv::ease::in,        lv::ease::out,          lv::ease::in_out,
    lv::ease::overshoot,    lv::ease::bounce,      lv::ease::step,      lv::ease::in_sine,      lv::ease::out_sine,
    lv::ease::in_out_sine,  lv::ease::in_quad,     lv::ease::out_quad,  lv::ease::in_out_quad,  lv::ease::in_cubic,
    lv::ease::out_cubic,    lv::ease::in_out_cubic, lv::ease::in_quart, lv::ease::out_quart,    lv::ease::in_out_quart,
    lv::ease::in_quint,     lv::ease::out_quint,   lv::ease::in_out_quint, lv::ease::in_expo,   lv::ease::out_expo,
    lv::ease::in_out_expo,  lv::ease::in_circ,     lv::ease::out_circ,  lv::ease::in_out_circ,  lv::ease::in_back,
    lv::ease::out_back,     lv::ease::in_out_back, lv::easing{line},    lv::easing{tok.motion.fall}};

// ---- parts: the concepts that constrain their parameters --------------------------

constexpr auto heading(const char* name, const lv::font_value auto& font) {
  return lv::label({.text = name, .style = {.text_font = font}});
}
constexpr auto filled(lv::token_of<lv_color_t> auto fill, lv::style_value auto extra) {
  return lv::box({.style = extra | lv::style({.bg_color = fill})});
}
constexpr auto framed(lv::tree_node auto content) {
  return lv::group[content];
}
constexpr auto stepper(const char* name, const lv::dependency auto& count, lv::tree_handler auto fewer) {
  return lv::row[lv::label({.text = name}), lv::button({.on_clicked = fewer}), lv::label({.text = lv::bind(count, "{}")})];
}
// Clang (21) cannot read `template <lv::dependency auto& Count>`; the requires clause means the same.
template <auto& Count>
  requires lv::dependency<std::remove_cvref_t<decltype(Count)>>
constexpr auto counting() {
  return stepper("Count", Count, [] { Count.update([](lv::counter c) { return ++c; }); });
}

constexpr auto parts = framed(lv::column[heading("Parts", label_font), filled(tok.color.ink, card),
                                         stepper("Clicks", clicks, &reset), counting<clicks>()]);
static_assert(lv::tree_node<decltype(parts)> && lv::token_value<decltype(tok.color.ink)>);
static_assert(lv::token_of<decltype(tok.color.ink), lv_color_t> && lv::style_value<decltype(card)>);  // C API
static_assert(lv::font_value<decltype(label_font)> && lv::font_value<decltype(lv::raw(odd_font))> && lv::view_value<lv::obj>);
static_assert(lv::dependency<decltype(over_limit)> && lv::event_handler<decltype(&reset)> && lv::tree_handler<decltype(&reset)> && lv::state_value<bool>);

// ---- choosing by value ------------------------------------------------------------

constexpr const char* name_of(mode m) {
  return lv::match(m)(lv::is<mode::off>.then([] { return "Off"; }),
                      lv::is<mode::eco, mode::sport>.then([] { return "On"; }));
}
constexpr int level_of(int n) {
  return lv::match(n)(lv::is<1, 2>.then([] { return 0; }), lv::otherwise.then([](int other) { return other; }));
}

// ---- documents and agents ---------------------------------------------------------

using screens = lv::runtime<"obj", "label", "arc">;

int use_runtime(lv::obj parent) {
  auto loaded = screens::load(R"({"type": "label", "id": "title", "props": {"text": "Hi"}})", parent);
  if (!loaded) {
    const lv::load_failure why = loaded.error();
    return why.error == lv::load_error::malformed ? static_cast<int>(why.at.size()) : 1;
  }
  std::string text;
  loaded->dump([&](std::string_view piece) { text += piece; });
  screens::dump(loaded->root(), [&](std::string_view piece) { text += piece; });
  static_cast<void>(loaded->id_of(loaded->find("title")));
  static_cast<void>(loaded->apply(R"({"title": {"text": "Ho"}})"));

  const lv::mcp::options who{.name = "uses", .version = "1", .screenshot_directory = ""};
  static_assert(lv::mcp::handshake_versions.size() == 4);
  lv::mcp::server<"obj", "label"> agent{parent, who};
  agent.serve(stdin, stdout);
  return static_cast<int>(agent.handle("{}").size() + lv::mcp::protocol_version.size());
}

std::size_t use_json(std::string_view text) {
  const auto document = lv::json::value::parse(text);
  if (!document || !document->is(lv::json::kind::object)) return 0;
  const lv::json::member first = document->member_at(0);
  const lv::json::value list = (*document)["list"];
  std::size_t walked = 0;
  const lv::json::items<lv::json::member> members = document->members();
  for (const lv::json::member m : members) walked += m.key.size();
  for (const lv::json::value element : list.elements()) walked += element.text().size();
  if (walked == 0) return 1;
  std::string out;
  auto sink = [&](std::string_view piece) { out += piece; };
  static_assert(lv::json::sink<decltype(sink)>);
  lv::json::write_string(sink, first.key);
  lv::json::write_number(sink, 3);
  if (list.type() != lv::json::kind::array || !list.at(0).boolean() || list.at(1).number() > 1) return 1;
  return out.size() + list.size() + list.text().size() + first.text.size() + list.at(2).string().size();
}

// ---- repetition ---------------------------------------------------------------------

enum class subpage { none, print, scan };
struct subpage_icons {
  static constexpr char print[] = "P";
  static constexpr char scan[] = "S";
};
struct level {
  int percent;
};
constexpr level levels[] = {{90}, {50}};

constexpr auto tile = []<subpage Page> {
  return lv::button[lv::label({.text = lv::like<Page, subpage_icons>}), lv::label({.text = lv::name_of<Page>})];
};
static_assert(lv::repeat_part<decltype(tile)>);
constexpr auto home = lv::row[lv::label({.text = "header"}),
                              lv::for_each<subpage, lv::but<subpage::none>>(tile)];
constexpr auto gauges = lv::row[lv::for_each<levels>([]<level L> {
  return lv::bar({.range = {0, 100}, .value = {L.percent, false}});
})];

// Clang reads no annotation, so the members of a row type are repeated in a template that
// is never instantiated: GCC checks the name, and tests/for_each.cpp runs it.
template <class Row>
constexpr auto details_of() {
  return lv::column[lv::for_each_field<Row>([]<lv::field F> {
    return lv::row[lv::label({.text = F.caption}), lv::label({.text = F.text()})];
  })];
}
constexpr lv::caption<6> owner_caption{"owner"};
static_assert(owner_caption.sv() == "owner");

}  // namespace api_uses

// ---- a display that follows a state --------------------------------------------------

void follows_a_mode(lv::display& display, lv::state<lv::signal::mode>& mode) {
  display.follow(mode, {lv::signal::day, lv::signal::dark});
  mode = lv::signal::mode::dark;
}

// ---- looks --------------------------------------------------------------------------

// The project names its tokens and typeface once, before the first look.
template <>
inline constexpr lv::design_of<decltype(api_uses::tok)> lv::design<lv::override>{
    .tokens = api_uses::tok, .type = lv::typeface({{"Barlow", 500, api_uses::barlow_ttf}})};
// A design without fonts, and a second design for a part library.
struct plain_design {};
template <>
inline constexpr lv::design_of<decltype(api_uses::tok)> lv::design<plain_design>{.tokens = api_uses::tok};

// A font that a file of its own draws. A file that defines the object does so before it names it.
template <>
const lv::drawn_font lv::font_object<"text.label"> = lv::draw<"text.label">(api_uses::type);

namespace api_uses {

constexpr auto printed = lv::button({.style = lv::look<"tile"> | lv::look<"tile-print">});
constexpr auto listed = lv::look<"tile tile-print tile-ink-level">;
constexpr auto with_layer = lv::look<"tile"> | lv::on("checked", {.bg_opa = tok.opacity.half});
static_assert(lv::style_value<decltype(listed)> && lv::style_value<decltype(with_layer)>);
constexpr auto per_page = []<subpage Page> {
  return lv::button({.style = lv::look<"tile"> | lv::look_of<"tile", Page>});
};
constexpr auto pages = lv::row[lv::for_each<subpage, lv::but<subpage::none>>(per_page)];
template <class Design = lv::override>
constexpr auto chip = lv::box({.style = lv::look<"tile-print", Design>});
constexpr auto chips = lv::row[chip<>, chip<plain_design>];

// A font that another file draws: named here, defined in the file that draws it.
constexpr auto split_label = lv::font_of<"text.label">;
static_assert(std::same_as<decltype(split_label), const lv::token<const lv_font_t*>>);  // C API
constexpr auto split_text = lv::label({.text = "a", .style = {.text_font = split_label}});

}  // namespace api_uses
