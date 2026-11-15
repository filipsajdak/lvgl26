// The touch panel of a printer, after the printer demo of Slint
// (https://github.com/slint-ui/slint, demos/printerdemo: MIT, design by Matthias Grimme).
// See NOTICE.md for what is taken from it and under which licence.
//
//   printer                  opens a window
//   printer --shot <file> [copy|scan]   writes the home page, or that sub-page, as a PPM picture
//
// The port so far: the side bar and the pages it switches between, the home page with
// its four actions, and the sub-pages that slide in over it. Copy and Scan have their
// settings; Print and USB have only their header yet.
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
constexpr unsigned char back_png[] = {
#embed "images/back.png"
};
constexpr unsigned char minus_png[] = {
#embed "images/minus.png"
};
constexpr unsigned char plus_png[] = {
#embed "images/plus.png"
};
constexpr lv_image_dsc_t back_icon = lv::png(back_png);
constexpr lv_image_dsc_t minus_icon = lv::png(minus_png);
constexpr lv_image_dsc_t plus_icon = lv::png(plus_png);

// ---- state --------------------------------------------------------------------

enum class page { home, settings, ink };
constinit lv::state<page> active_page{page::home};
constinit lv::computed on_home{[](page p) { return p != page::home; }, active_page};
constinit lv::computed on_settings{[](page p) { return p != page::settings; }, active_page};
constinit lv::computed on_ink{[](page p) { return p != page::ink; }, active_page};

// The page that slides in over the home page, if any.
enum class subpage { none, print, scan, copy, usb };
constinit lv::state<subpage> active_subpage{subpage::none};
void open_print() { active_subpage = subpage::print; }
void open_scan() { active_subpage = subpage::scan; }
void open_copy() { active_subpage = subpage::copy; }
void open_usb() { active_subpage = subpage::usb; }
void close_subpage() { active_subpage = subpage::none; }

// What the Copy and the Scan page set.
constinit lv::state copies{lv::counter{1}};
constinit lv::state scans{lv::counter{1}};
constinit lv::state<bool> copy_in_color{true};
constinit lv::state<bool> scan_in_color{true};
constinit lv::computed copy_color_name{[](bool color) { return color ? "Color" : "Grayscale"; }, copy_in_color};
constinit lv::computed scan_color_name{[](bool color) { return color ? "Color" : "Grayscale"; }, scan_in_color};

constexpr lv::counter one_fewer(lv::counter n) { return n.value > 1 ? lv::counter{n.value - 1} : n; }
constexpr lv::counter one_more(lv::counter n) { return n.value < 100 ? lv::counter{n.value + 1} : n; }
void fewer_copies() { copies.update(&one_fewer); }
void more_copies() { copies.update(&one_more); }
void fewer_scans() { scans.update(&one_fewer); }
void more_scans() { scans.update(&one_more); }
void toggle_copy_color() { copy_in_color = !copy_in_color.get(); }
void toggle_scan_color() { scan_in_color = !scan_in_color.get(); }

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
constexpr int page_width = window_width - sidebar_width;
constexpr int header_height = 102;
constexpr int item_width = (page_width - 16 - 2 * margin) / 2;

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
constexpr auto action(const char* name, const lv_image_dsc_t* picture, const Colors& colors, void (*open)()) {
  return lv::button({.on_clicked = open,
                     .size = {tile_width * px, tile_height * px},
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

// A small square button with a picture: back, fewer, more.
constexpr auto icon_button(const lv_image_dsc_t* picture, void (*clicked)()) {
  return lv::button({.on_clicked = clicked,
                     .size = {40 * px, 40 * px},
                     .style = {.bg_color = tok.spinbox.button, .bg_opa = tok.solid, .radius = tok.spinbox.radius,
                               .shadow_width = lv::raw(0 * px)}})[
      lv::image({.align = LV_ALIGN_CENTER,
                 .src = picture,
                 .style = {.image_recolor = tok.spinbox.text, .image_recolor_opa = tok.tint}})];
}

// The header of a sub-page: back, its title, and a line of what to do there.
constexpr auto back_header(const char* title, const char* subtitle) {
  return lv::group({.size = {100 * pct, header_height * px},
                    .style = {.bg_color = tok.header.background, .bg_opa = tok.solid}})[
      lv::group({.size = {40 * px, 40 * px}, .x = margin * px, .y = 16 * px})[icon_button(&back_icon, &close_subpage)],
      lv::label({.style = {.text_color = tok.text.primary}, .text = title, .x = (margin + 52) * px, .y = 24 * px}),
      lv::label({.style = {.text_color = tok.text.tertiary}, .text = subtitle, .x = (margin + 52) * px, .y = 62 * px}),
      lv::group({.size = {100 * pct, 2 * px},
                 .style = {.bg_color = tok.header.separator, .bg_opa = tok.solid},
                 .y = (header_height - 2) * px})];
}

// A number with a button on each side: "Copies  [-] 1 [+]".
template <class Count>
constexpr auto spin_box(const char* name, Count& count, void (*fewer)(), void (*more)()) {
  return lv::group({.size = {item_width * px, 40 * px}})[
      lv::label({.align = LV_ALIGN_LEFT_MID, .style = {.text_color = tok.text.primary}, .text = name}),
      lv::group({.size = {40 * px, 40 * px}, .x = 90 * px})[icon_button(&minus_icon, fewer)],
      lv::label({.align = LV_ALIGN_LEFT_MID, .style = {.text_color = tok.text.primary},
                 .text = lv::bind(count, "{}"), .x = 146 * px}),
      lv::group({.size = {40 * px, 40 * px}, .x = 182 * px})[icon_button(&plus_icon, more)]];
}

// A choice from a list, with its name above it.
constexpr auto choice(const char* name, const char* options) {
  return lv::group({.size = {item_width * px, 66 * px}})[
      lv::label({.style = {.text_color = tok.text.tertiary}, .text = name}),
      lv::dropdown({.options = options, .size = {item_width * px, 40 * px}, .y = 24 * px})];
}

// The settings of a Copy or a Scan: how many, color or not, four choices, and start.
template <class Count, class Name>
constexpr auto settings(Count& count, void (*fewer)(), void (*more)(), Name& color_name, void (*toggle)(),
                        const char* second_name, const char* second_options, const char* start) {
  constexpr int right = margin + item_width + 16;
  return lv::group({.size = {100 * pct, (window_height - header_height) * px}, .y = header_height * px})[
      lv::group({.size = {item_width * px, 40 * px}, .x = margin * px, .y = margin * px})[
          spin_box("Copies", count, fewer, more)],
      lv::button({.on_clicked = toggle,
                  .size = {item_width * px, 40 * px},
                  .style = {.bg_color = tok.button.color.from, .bg_grad_color = tok.button.color.to,
                            .bg_grad_dir = LV_GRAD_DIR_HOR, .bg_opa = tok.solid, .radius = tok.button.radius,
                            .shadow_width = lv::raw(0 * px)},
                  .x = right * px,
                  .y = margin * px})[
          lv::label({.align = LV_ALIGN_CENTER, .style = {.text_color = tok.button.text},
                     .text = lv::bind(color_name)})],
      lv::group({.size = {item_width * px, 66 * px}, .x = margin * px, .y = (margin + 56) * px})[
          choice("Quality", "Best\nMedium\nDraft")],
      lv::group({.size = {item_width * px, 66 * px}, .x = right * px, .y = (margin + 56) * px})[
          choice(second_name, second_options)],
      lv::group({.size = {item_width * px, 66 * px}, .x = margin * px, .y = (margin + 138) * px})[
          choice("Paper Size", "Original\nA4")],
      lv::group({.size = {item_width * px, 66 * px}, .x = right * px, .y = (margin + 138) * px})[
          choice("Layout", "Portrait\nLandscape")],
      lv::button({.on_clicked = &close_subpage,
                  .size = {(page_width - 2 * margin) * px, 60 * px},
                  .style = {.bg_color = tok.button.primary, .bg_opa = tok.solid, .radius = tok.button.radius,
                            .shadow_width = lv::raw(0 * px)},
                  .x = margin * px,
                  .y = (margin + 228) * px})[
          lv::label({.align = LV_ALIGN_CENTER, .style = {.text_color = tok.button.text}, .text = start})]];
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
                action("Print", &print_icon, tok.tile.print, &open_print)],
            lv::group({.size = {tile_width * px, tile_height * px}, .x = (tile_width + 16) * px})[
                action("Scan", &scan_icon, tok.tile.scan, &open_scan)],
            lv::group({.size = {tile_width * px, tile_height * px}, .y = (tile_height + 16) * px})[
                action("Copy", &copy_icon, tok.tile.copy, &open_copy)],
            lv::group({.size = {tile_width * px, tile_height * px},
                       .x = (tile_width + 16) * px,
                       .y = (tile_height + 16) * px})[
                action("USB", &usb_icon, tok.tile.usb, &open_usb)]],
        lv::group({.id = "queue",
                   .size = {(window_width - sidebar_width - 3 * margin - 2 * tile_width - 16) * px, 100 * pct},
                   .style = {.bg_color = tok.queue.background, .bg_opa = tok.solid},
                   .x = (2 * margin + 2 * tile_width + 16) * px})[
            lv::label({.style = {.text_color = tok.text.tertiary}, .text = "Print queue", .x = margin * px,
                       .y = margin * px})],
        // The sub-pages wait beside the page, out of sight, and slide in over it.
        lv::group({.id = "print_page", .style = {.bg_color = tok.window.background, .bg_opa = tok.solid},
                   .x = page_width * px})[back_header("Print", "Printing-Queue")],
        lv::group({.id = "scan_page", .style = {.bg_color = tok.window.background, .bg_opa = tok.solid},
                   .x = page_width * px})[
            back_header("Scan", "Choose Settings"),
            settings(scans, &fewer_scans, &more_scans, scan_color_name, &toggle_scan_color, "Format",
                     "PDF\nJPEG\nPNG", "Start scanning")],
        lv::group({.id = "copy_page", .style = {.bg_color = tok.window.background, .bg_opa = tok.solid},
                   .x = page_width * px})[
            back_header("Copy", "Choose Settings"),
            settings(copies, &fewer_copies, &more_copies, copy_color_name, &toggle_copy_color, "Size",
                     "100%\n75%\n50%", "Start copying")],
        lv::group({.id = "usb_page", .style = {.bg_color = tok.window.background, .bg_opa = tok.solid},
                   .x = page_width * px})[back_header("USB", "Choose Settings")]],
    lv::group({.hidden = lv::bind(on_settings),
               .id = "settings",
               .size = {(window_width - sidebar_width) * px, 100 * pct},
               .x = sidebar_width * px})[placeholder("Settings")],
    lv::group({.hidden = lv::bind(on_ink),
               .id = "ink",
               .size = {(window_width - sidebar_width) * px, 100 * pct},
               .x = sidebar_width * px})[placeholder("Ink levels")]];

// ---- sliding the sub-pages ----------------------------------------------------

// The sub-pages as widgets, in the order of `subpage` after `none`.
lv::view<"obj"> subpage_views[4];
subpage shown = subpage::none;

lv::view<"obj"> view_of(subpage s) { return subpage_views[static_cast<int>(s) - 1]; }

lv::task<> slide(lv::view<"obj"> page_view, int from, int to) {
  co_await lv::animate(page_view, {.x = {from * px, to * px}}, 125 * ms, tok.motion.slide);
}

// Slides the page that was shown out and the new one in.
void show(subpage next) {
  if (next == shown) return;
  if (shown != subpage::none) lv::spawn(slide(view_of(shown), 0, page_width));
  if (next != subpage::none) lv::spawn(slide(view_of(next), page_width, 0));
  shown = next;
}
constinit lv::effect follow_subpage{[](subpage s) { show(s); }, active_subpage};

// Builds the screen and starts what follows the state.
bool start(lv::obj screen) {
  static auto root = lv::mount<ui>(screen);
  if (!root) return false;
  subpage_views[0] = root->get<"print_page">();
  subpage_views[1] = root->get<"scan_page">();
  subpage_views[2] = root->get<"copy_page">();
  subpage_views[3] = root->get<"usb_page">();
  follow_subpage.start();
  return true;
}

// ---- running it ---------------------------------------------------------------

void flush(lv_display_t* display, const lv_area_t*, std::uint8_t*) { lv_display_flush_ready(display); }

int shot(const char* file, std::string_view page_name) {
  static std::vector<std::uint8_t> pixels(window_width * window_height * 4);
  lv_display_t* display = lv_display_create(window_width, window_height);
  lv_display_set_buffers(display, pixels.data(), nullptr, static_cast<std::uint32_t>(pixels.size()),
                         LV_DISPLAY_RENDER_MODE_FULL);
  lv_display_set_flush_cb(display, &flush);
  if (!start(lv::screen_active())) return 1;
  if (page_name == "copy") open_copy();
  if (page_name == "scan") open_scan();
  for (int passed = 0; passed < 400; passed += 10) {  // long enough for a page to slide in
    lv_tick_inc(10);
    lv_timer_handler();
  }
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
  if (!start(lv::screen_active())) return 1;
  for (;;) {
    const std::uint32_t wait = lv_timer_handler();
    std::this_thread::sleep_for(std::chrono::milliseconds(wait < 1 ? 1 : wait > 4 ? 4 : wait));
  }
}

}  // namespace

int main(int argc, char** argv) {
  lv_init();
  if (argc >= 3 && std::string_view(argv[1]) == "--shot") return shot(argv[2], argc == 4 ? argv[3] : "");
  return show();
}
