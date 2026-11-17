// The Settings page: two groups of options, with check boxes and choices.

#include "fonts.hpp"
#include "pages.hpp"
#include "parts.hpp"

namespace printer {
namespace {

constinit lv::state<bool> turbo{true};
constinit lv::state<bool> eco{false};
constinit lv::state<bool> sort_pages{true};
constexpr bool is_off(bool on) { return !on; }
constinit lv::computed turbo_off{&is_off, turbo};
constinit lv::computed eco_off{&is_off, eco};
constinit lv::computed sort_off{&is_off, sort_pages};

void toggle_turbo() { turbo = !turbo.get(); }
void toggle_eco() { eco = !eco.get(); }
void toggle_sort() { sort_pages = !sort_pages.get(); }

constexpr int plain_header_height = 86;
constexpr int box_size = 32;
constexpr int label_x = margin;
constexpr int control_x = 170;
constexpr int control_width = 170;
constexpr int second_label_x = 350;
constexpr int second_control_x = 530;
constexpr int second_width = page_width - margin - second_control_x;
constexpr int heading_step = 22 + gap;
constexpr int row_general = plain_header_height + margin + heading_step;
constexpr int row_defaults = row_general + box_size + gap;
constexpr int row_layout = row_defaults + heading_step;
constexpr int row_quality = row_layout + 52 + gap;
constexpr int row_color = row_quality + 52 + gap;

constexpr auto title_font = type(tok.text.title);
constexpr auto label_font = type(tok.text.label);
constexpr auto body_font = type(tok.text.body);

// A check box: a square with a tick, and its name. The page owns the state that it follows.
template <class Unchecked>
constexpr auto check_box(const char* name, const Unchecked& unchecked, void (*toggle)(), int x, int y) {
  return lv::group({.size = {control_width * px, box_size * px}, .x = x * px, .y = y * px})[
      lv::button({.on_clicked = toggle,
                  .size = {box_size * px, box_size * px},
                  .style = {.bg_color = tok.control.background, .bg_opa = tok.solid,
                            .border_color = tok.control.border, .border_width = tok.control.line,
                            .radius = tok.queue.dot, .shadow_width = lv::raw(0 * px)}})[
          lv::image({.align = "center", .hidden = lv::bind(unchecked), .src = lv::image_file<icons::check>,
                     .style = {.image_recolor = tok.ink.accent, .image_recolor_opa = tok.tint}})],
      lv::label({.align = "left_mid", .style = {.text_color = tok.label.primary, .text_font = body_font},
                 .text = name, .x = (box_size + gap) * px})];
}

constexpr auto heading(const char* text, int y) {
  return lv::label({.style = {.text_color = tok.color.muted, .text_font = label_font}, .text = text,
                    .x = margin * px, .y = y * px});
}

constexpr auto name(const char* text, int x, int y) {
  return lv::label({.style = {.text_color = tok.label.primary, .text_font = label_font}, .text = text,
                    .x = x * px, .y = (y + 14) * px});
}

constexpr auto drop(const char* options, int x, int y, int width) {
  return lv::dropdown({.options = options,
                       .size = {width * px, 52 * px},
                       .style = {.bg_color = tok.control.background, .bg_opa = tok.solid,
                                 .border_color = tok.control.border, .border_width = tok.control.line,
                                 .image_recolor = tok.label.primary, .image_recolor_opa = tok.tint,
                                 .radius = tok.control.radius, .text_font = body_font},
                       .symbol = lv::image_file<icons::down>,
                       .x = x * px,
                       .y = y * px});
}

constexpr auto ui = lv::group({.size = {100 * pct, 100 * pct},
                               .style = {.bg_color = tok.window.background, .bg_opa = tok.solid}})[
    lv::group({.size = {100 * pct, plain_header_height * px},
               .style = {.bg_color = tok.header.background, .bg_opa = tok.solid}})[
        lv::label({.style = {.text_color = tok.label.primary, .text_font = title_font}, .text = "Settings",
                   .x = margin * px, .y = 28 * px}),
        lv::group({.size = {100 * pct, 2 * px},
                   .style = {.bg_color = tok.header.separator, .bg_opa = tok.solid},
                   .y = (plain_header_height - 2) * px})],
    heading("General", plain_header_height + margin),
    name("TURBO mode", label_x, row_general - 14), check_box("", turbo_off, &toggle_turbo, control_x, row_general),
    name("Power Management", second_label_x, row_general - 14),
    check_box("Eco Mode", eco_off, &toggle_eco, second_control_x, row_general),
    heading("Defaults", row_defaults),
    name("Layout", label_x, row_layout), drop("Portrait\nLandscape", control_x, row_layout, control_width),
    name("Paper Tray", second_label_x, row_layout), drop("Special Tray\nTray 1\nTray 2", second_control_x, row_layout, second_width),
    name("Quality", label_x, row_quality), drop("Best\nMedium\nDraft", control_x, row_quality, control_width),
    name("Language", second_label_x, row_quality), drop("English\nFrench", second_control_x, row_quality, second_width),
    name("Color Mode", label_x, row_color), drop("Grayscale\nColor", control_x, row_color, control_width),
    name("Paper Handling", second_label_x, row_color),
    check_box("Sort Pages", sort_off, &toggle_sort, second_control_x, row_color + 10)];

}  // namespace

lv::owned<lv::obj> mount_settings(lv::obj parent) { return mount_page<ui>(parent); }

}  // namespace printer
