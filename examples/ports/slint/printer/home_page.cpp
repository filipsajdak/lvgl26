// The home page: four actions, and the print queue beside them.

#include "fonts.hpp"
#include "pages.hpp"
#include "parts.hpp"
#include "queue_column.hpp"

namespace printer {
namespace {

constexpr int inner_size = 64;
constexpr int name_height = 22;  // the picture, the gap and the name make a block of 96 px, centered in the tile
constexpr int tiles_width = 2 * tile_width + gap;
constexpr int tiles_height = 2 * tile_height + gap;

constexpr auto title_font = type(tok.text.title);
constexpr auto label_font = type(tok.text.label);

// One of the four actions: a colored tile with a picture and a name, centered as one block.
template <const auto& Icon, class Colors>
constexpr auto tile(const char* name, const Colors& colors, void (*open)()) {
  return lv::button({.flex_align = {"center", "center", "center"}, .flex_flow = "column", .on_clicked = open,
                     .size = {tile_width * px, tile_height * px},
                     .style = lv::style({.bg_color = colors.from, .bg_grad_color = colors.to, .bg_grad_dir = "ver",
                                         .bg_opa = tok.solid, .pad_all = tok.space.none, .pad_gap = tok.tile.gap,
                                         .radius = tok.tile.radius, .shadow_width = no_shadow})})[
      lv::group({.size = {inner_size * px, inner_size * px},
                 .style = {.bg_color = tok.tile.inner, .bg_opa = tok.tile.inner_strength,
                           .radius = tok.tile.inner_radius}})[picture<Icon>(tok.label.on_tile)],
      lv::label({.size = {tile_width * px, name_height * px},
                 .style = {.text_align = "center", .text_color = tok.label.on_tile, .text_font = label_font},
                 .text = name})];
}

constexpr auto ui = lv::group({.size = {100 * pct, 100 * pct}})[
    lv::label({.style = {.text_color = tok.label.primary, .text_font = title_font},
               .text = "Printer", .x = margin * px, .y = margin * px}),
    lv::group({.flex_flow = "row_wrap", .size = {tiles_width * px, tiles_height * px},
               .style = {.pad_gap = tok.space.card},
               .x = margin * px,
               .y = (window_height - margin - tiles_height) * px})[
        tile<icons::print>("Print", tok.tile.print, &open_print), tile<icons::scan>("Scan", tok.tile.scan, &open_scan),
        tile<icons::copy>("Copy", tok.tile.copy, &open_copy), tile<icons::usb>("USB", tok.tile.usb, &open_usb)],
    queue_part::queue_column];

}  // namespace

lv::owned<lv::obj> mount_home(lv::obj parent) { return mount_page<ui>(parent); }

}  // namespace printer
