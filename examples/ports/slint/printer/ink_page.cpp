// The Ink page: four tubes that fill to the level of each ink.

#include "fonts.hpp"
#include "pages.hpp"
#include "parts.hpp"

namespace printer {
namespace {

constexpr int plain_header_height = 86;
constexpr int tube_gap = 50;
constexpr int tube_width = (page_width * 3 / 4 - 3 * tube_gap) / 4;
constexpr int tube_height = (window_height - plain_header_height) * 65 / 100;
constexpr int tubes_left = (page_width - (4 * tube_width + 3 * tube_gap)) / 2;
constexpr int tubes_top = plain_header_height + (window_height - plain_header_height - tube_height) / 2;
constexpr int pill_height = 30;

constexpr auto title_font = type(tok.text.title);
constexpr auto small_font = type(tok.text.small);

// One tube: its outline, the ink that rises in it, and the level written on a label.
template <class Ink>
constexpr auto tube(std::string_view fill_id, const Ink& ink, const char* percent, int index) {
  return lv::group({.size = {tube_width * px, tube_height * px},
                    .x = (tubes_left + index * (tube_width + tube_gap)) * px,
                    .y = tubes_top * px})[
      lv::group({.align = "bottom_mid", .id = fill_id, .size = {tube_width * px, 0 * px}})[
          lv::group({.align = "bottom_mid", .size = {tube_width * px, tube_height * px},
                     .style = {.bg_color = ink, .bg_opa = tok.solid, .radius = tok.ink.radius}})],
      lv::group({.size = {100 * pct, 100 * pct},
                 .style = {.border_color = tok.color.neutral, .border_width = tok.control.line,
                           .radius = tok.ink.radius}}),
      lv::group({.align = "bottom_mid", .size = {tube_width / 2 * px, pill_height * px},
                 .style = {.bg_color = tok.ink.pill, .bg_opa = tok.ink.pill_opa, .radius = tok.ink.radius},
                 .y = -pill_height * px})[
          lv::label({.align = "center", .style = {.text_color = tok.ink.text, .text_font = small_font},
                     .text = percent})]];
}

constexpr int levels[] = {90, 50, 80, 10};

constexpr auto ui = lv::group({.size = {100 * pct, 100 * pct},
                               .style = {.bg_color = tok.window.background, .bg_opa = tok.solid}})[
    lv::group({.size = {100 * pct, plain_header_height * px},
               .style = {.bg_color = tok.header.background, .bg_opa = tok.solid}})[
        lv::label({.style = {.text_color = tok.label.primary, .text_font = title_font}, .text = "Ink Level",
                   .x = margin * px, .y = 28 * px}),
        lv::group({.size = {100 * pct, 2 * px},
                   .style = {.bg_color = tok.header.separator, .bg_opa = tok.solid},
                   .y = (plain_header_height - 2) * px})],
    tube("yellow", tok.ink.yellow, "90%", 0), tube("cyan", tok.ink.cyan, "50%", 1),
    tube("magenta", tok.ink.magenta, "80%", 2), tube("black", tok.ink.black, "10%", 3)];

// The ink rises to its level, a moment after the page appears.
lv::task<> fill(lv::obj ink, int level) {
  co_await lv::animate(ink, {.height = {0 * px, tube_height * level / 100 * px}}, tok.motion.ink_time,
                       tok.motion.ink);
}

lv::task<> rise(lv::obj a, lv::obj b, lv::obj c, lv::obj d) {
  co_await lv::sleep(tok.motion.ink_delay);
  lv::spawn(fill(a, levels[0]), a);
  lv::spawn(fill(b, levels[1]), b);
  lv::spawn(fill(c, levels[2]), c);
  lv::spawn(fill(d, levels[3]), d);
}

}  // namespace

lv::owned<lv::obj> mount_ink(lv::obj parent) {
  auto built = lv::mount<ui>(parent);
  if (!built) return {};
  lv::spawn(rise(lv::obj{built->get<"yellow">().raw()}, lv::obj{built->get<"cyan">().raw()},
                 lv::obj{built->get<"magenta">().raw()}, lv::obj{built->get<"black">().raw()}),
            built->root());
  return lv::owned<lv::obj>{built->detach()};
}

}  // namespace printer
