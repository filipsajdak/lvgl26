// The touch panel of a printer, after the printer demo of Slint
// (https://github.com/slint-ui/slint, demos/printerdemo: MIT, design by Matthias Grimme).
// See NOTICE.md for what is taken from it and under which licence.
//
//   printer                  opens a window
//   printer --shot <file>    writes the home page to <file> as a PPM picture
//
// This is the first stage of the port: the side bar, the pages it switches between,
// and the home page with its four actions.
#include <lv/lv.hpp>

#include <chrono>
#include <cstdio>
#include <string_view>
#include <thread>
#include <vector>

namespace {

using namespace lv::units;

constexpr char tokens_json[] = {
#embed "tokens.json"
    , 0};
constexpr auto tok = lv::tokens<tokens_json>;

// ---- pictures ---------------------------------------------------------------

constexpr unsigned char home_png[] = {
#embed "images/home.png"
};
constexpr unsigned char settings_png[] = {
#embed "images/settings.png"
};
constexpr unsigned char ink_png[] = {
#embed "images/ink.png"
};
constexpr unsigned char print_png[] = {
#embed "images/print.png"
};
constexpr unsigned char scan_png[] = {
#embed "images/scan.png"
};
constexpr unsigned char copy_png[] = {
#embed "images/copy.png"
};
constexpr unsigned char usb_png[] = {
#embed "images/usb.png"
};
constexpr lv_image_dsc_t home_icon = lv::png(home_png);
constexpr lv_image_dsc_t settings_icon = lv::png(settings_png);
constexpr lv_image_dsc_t ink_icon = lv::png(ink_png);
constexpr lv_image_dsc_t print_icon = lv::png(print_png);
constexpr lv_image_dsc_t scan_icon = lv::png(scan_png);
constexpr lv_image_dsc_t copy_icon = lv::png(copy_png);
constexpr lv_image_dsc_t usb_icon = lv::png(usb_png);

// ---- state --------------------------------------------------------------------

enum class page { home, settings, ink };
constinit lv::state<page> active_page{page::home};
constinit lv::computed on_home{[](page p) { return p != page::home; }, active_page};
constinit lv::computed on_settings{[](page p) { return p != page::settings; }, active_page};
constinit lv::computed on_ink{[](page p) { return p != page::ink; }, active_page};

void go_home() { active_page = page::home; }
void go_settings() { active_page = page::settings; }
void go_ink() { active_page = page::ink; }

// ---- sizes, after the original ------------------------------------------------

constexpr int window_width = 772;
constexpr int window_height = 504;
constexpr int sidebar_width = 64;
constexpr int margin = 24;
constexpr int tile_width = 162;
constexpr int tile_height = 192;

// ---- components ---------------------------------------------------------------

// A side bar icon: a rounded square with a picture, tinted by the theme.
constexpr auto side_icon(const lv_image_dsc_t* picture, void (*go)()) {
  return lv::button({.on_clicked = go,
                     .size = {40 * px, 40 * px},
                     .style = {.bg_color = tok.sidebar.active, .bg_opa = tok.solid,
                               .radius = tok.sidebar.icon_radius, .shadow_width = lv::raw(0 * px)}})[
      lv::image({.align = LV_ALIGN_CENTER,
                 .src = picture,
                 .style = {.image_recolor = tok.sidebar.icon_active, .image_recolor_opa = tok.tint}})];
}

// One of the four actions of the home page: a colored tile with a picture and a name.
template <class Colors>
constexpr auto action(const char* name, const lv_image_dsc_t* picture, const Colors& colors) {
  return lv::button({.size = {tile_width * px, tile_height * px},
                     .style = {.bg_color = colors.from, .bg_grad_color = colors.to,
                               .bg_grad_dir = LV_GRAD_DIR_VER, .bg_opa = tok.solid,
                               .radius = tok.tile.radius, .shadow_width = lv::raw(0 * px)}})[
      lv::group({.align = LV_ALIGN_CENTER,
                 .size = {64 * px, 64 * px},
                 .style = {.bg_color = tok.tile.inner, .bg_opa = tok.tile.inner_strength,
                           .radius = tok.tile.inner_radius},
                 .y = -20 * px})[
          lv::image({.align = LV_ALIGN_CENTER,
                     .src = picture,
                     .style = {.image_recolor = tok.text.on_tile, .image_recolor_opa = tok.tint}})],
      lv::label({.align = LV_ALIGN_CENTER,
                 .style = {.text_color = tok.text.on_tile},
                 .text = name,
                 .y = 42 * px})];
}

// A page that is not ported yet: its name, where the page will be.
constexpr auto placeholder(const char* title) {
  return lv::label({.style = {.text_color = tok.text.tertiary}, .text = title, .x = margin * px, .y = margin * px});
}

// ---- the screen ---------------------------------------------------------------

constexpr auto ui = lv::group({.style = {.bg_color = tok.window.background, .bg_opa = tok.solid}})[
    lv::group({.id = "sidebar",
               .size = {sidebar_width * px, 100 * pct},
               .style = {.bg_color = tok.sidebar.background, .bg_opa = tok.solid}})[
        lv::group({.align = LV_ALIGN_TOP_MID, .size = {40 * px, 40 * px}, .y = margin * px})[
            side_icon(&home_icon, &go_home)],
        lv::group({.align = LV_ALIGN_TOP_MID, .size = {40 * px, 40 * px}, .y = (margin + 64) * px})[
            side_icon(&settings_icon, &go_settings)],
        lv::group({.align = LV_ALIGN_TOP_MID, .size = {40 * px, 40 * px}, .y = (margin + 128) * px})[
            side_icon(&ink_icon, &go_ink)]],
    lv::group({.hidden = lv::bind(on_home),
               .id = "home",
               .size = {(window_width - sidebar_width) * px, 100 * pct},
               .x = sidebar_width * px})[
        lv::label({.style = {.text_color = tok.text.primary}, .text = "Printer", .x = margin * px, .y = margin * px}),
        lv::group({.size = {(2 * tile_width + 16) * px, (2 * tile_height + 16) * px},
                   .x = margin * px,
                   .y = (window_height - margin - 2 * tile_height - 16) * px})[
            lv::group({.size = {tile_width * px, tile_height * px}})[
                action("Print", &print_icon, tok.tile.print)],
            lv::group({.size = {tile_width * px, tile_height * px}, .x = (tile_width + 16) * px})[
                action("Scan", &scan_icon, tok.tile.scan)],
            lv::group({.size = {tile_width * px, tile_height * px}, .y = (tile_height + 16) * px})[
                action("Copy", &copy_icon, tok.tile.copy)],
            lv::group({.size = {tile_width * px, tile_height * px},
                       .x = (tile_width + 16) * px,
                       .y = (tile_height + 16) * px})[
                action("USB", &usb_icon, tok.tile.usb)]],
        lv::group({.id = "queue",
                   .size = {(window_width - sidebar_width - 3 * margin - 2 * tile_width - 16) * px, 100 * pct},
                   .style = {.bg_color = tok.queue.background, .bg_opa = tok.solid},
                   .x = (2 * margin + 2 * tile_width + 16) * px})[
            lv::label({.style = {.text_color = tok.text.tertiary}, .text = "Print queue", .x = margin * px,
                       .y = margin * px})]],
    lv::group({.hidden = lv::bind(on_settings),
               .id = "settings",
               .size = {(window_width - sidebar_width) * px, 100 * pct},
               .x = sidebar_width * px})[placeholder("Settings")],
    lv::group({.hidden = lv::bind(on_ink),
               .id = "ink",
               .size = {(window_width - sidebar_width) * px, 100 * pct},
               .x = sidebar_width * px})[placeholder("Ink levels")]];

// ---- running it ---------------------------------------------------------------

void flush(lv_display_t* display, const lv_area_t*, std::uint8_t*) { lv_display_flush_ready(display); }

int shot(const char* file) {
  static std::vector<std::uint8_t> pixels(window_width * window_height * 4);
  lv_display_t* display = lv_display_create(window_width, window_height);
  lv_display_set_buffers(display, pixels.data(), nullptr, static_cast<std::uint32_t>(pixels.size()),
                         LV_DISPLAY_RENDER_MODE_FULL);
  lv_display_set_flush_cb(display, &flush);
  lv::place<ui>(lv::screen_active());
  lv_tick_inc(20);
  lv_timer_handler();
  lv_draw_buf_t* picture = lv_snapshot_take(lv_screen_active(), LV_COLOR_FORMAT_ARGB8888);
  std::FILE* out = picture != nullptr ? std::fopen(file, "wb") : nullptr;
  if (out == nullptr) return 1;
  std::fprintf(out, "P6\n%d %d\n255\n", static_cast<int>(picture->header.w), static_cast<int>(picture->header.h));
  for (std::uint32_t y = 0; y < picture->header.h; ++y) {
    const std::uint8_t* row = picture->data + y * picture->header.stride;
    for (std::uint32_t x = 0; x < picture->header.w; ++x) {
      const std::uint8_t rgb[3] = {row[x * 4 + 2], row[x * 4 + 1], row[x * 4]};
      std::fwrite(rgb, 1, 3, out);
    }
  }
  std::fclose(out);
  lv_draw_buf_destroy(picture);
  return 0;
}

int show() {
  lv_sdl_window_create(window_width, window_height);
  lv_sdl_mouse_create();
  lv::place<ui>(lv::screen_active());
  for (;;) {
    const std::uint32_t wait = lv_timer_handler();
    std::this_thread::sleep_for(std::chrono::milliseconds(wait < 1 ? 1 : wait > 4 ? 4 : wait));
  }
}

}  // namespace

int main(int argc, char** argv) {
  lv_init();
  if (argc == 3 && std::string_view(argv[1]) == "--shot") return shot(argv[2]);
  return show();
}
