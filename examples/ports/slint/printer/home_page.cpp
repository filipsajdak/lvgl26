// The home page: four actions, and the print queue beside them.

#include "fonts.hpp"
#include "pages.hpp"
#include "parts.hpp"

namespace printer {
namespace {

constexpr int inner_size = 64;
constexpr int block_top = 48;  // the picture and the name are centered in the tile, 96 px high together
constexpr int tiles_width = 2 * tile_width + gap;
constexpr int tiles_height = 2 * tile_height + gap;

constexpr auto title_font = type(tok.text.title);
constexpr auto label_font = type(tok.text.label);

// One of the four actions: a colored tile with a picture and a name.
template <const auto& Icon, class Colors>
constexpr auto tile(const char* name, const Colors& colors, void (*open)(), int column, int row) {
  return lv::group({.size = {tile_width * px, tile_height * px},
                    .x = column * (tile_width + gap) * px,
                    .y = row * (tile_height + gap) * px})[
      lv::button({.on_clicked = open,
                  .size = {tile_width * px, tile_height * px},
                  .style = lv::style({.bg_color = colors.from, .bg_grad_color = colors.to, .bg_grad_dir = "ver",
                                      .bg_opa = tok.solid, .pad_all = tok.space.none, .radius = tok.tile.radius,
                                      .shadow_width = no_shadow})})[
          lv::group({.size = {inner_size * px, inner_size * px},
                     .style = {.bg_color = tok.tile.inner, .bg_opa = tok.tile.inner_strength,
                               .radius = tok.tile.inner_radius},
                     .x = (tile_width - inner_size) / 2 * px,
                     .y = block_top * px})[picture<Icon>(tok.label.on_tile)],
          lv::label({.size = {tile_width * px, lv::size_content},
                     .style = {.text_align = "center", .text_color = tok.label.on_tile, .text_font = label_font},
                     .text = name,
                     .y = (block_top + inner_size + 10) * px})]];
}

constexpr auto ui = lv::group({.size = {100 * pct, 100 * pct}})[
    lv::label({.style = {.text_color = tok.label.primary, .text_font = title_font},
               .text = "Printer", .x = margin * px, .y = margin * px}),
    lv::group({.size = {tiles_width * px, tiles_height * px},
               .x = margin * px,
               .y = (window_height - margin - tiles_height) * px})[
        tile<icons::print>("Print", tok.tile.print, &open_print, 0, 0),
        tile<icons::scan>("Scan", tok.tile.scan, &open_scan, 1, 0),
        tile<icons::copy>("Copy", tok.tile.copy, &open_copy, 0, 1),
        tile<icons::usb>("USB", tok.tile.usb, &open_usb, 1, 1)]];

}  // namespace

lv::owned<lv::obj> mount_home(lv::obj parent) {
  lv::owned<lv::obj> page = mount_page<ui>(parent);
  if (page) {
    lv::owned<lv::obj> column = mount_queue_view(page.get());
    column.release();  // the queue lives and dies with the page
  }
  return page;
}

}  // namespace printer
