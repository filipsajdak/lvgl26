// The USB page: a photo from a stick, which file, how many copies, color or not, and print.
// The photos are cropped to fit; their credits are in NOTICE.md.

#include "fonts.hpp"
#include "pages.hpp"
#include "parts.hpp"
#include "queue.hpp"

#include <array>

namespace printer {
namespace {

constexpr unsigned char cat_jpg[] = {
#embed "photos/cat.jpg"
};
constexpr unsigned char dog_jpg[] = {
#embed "photos/dog.jpg"
};
constexpr unsigned char elephant_jpg[] = {
#embed "photos/elephant.jpg"
};
constexpr unsigned char snake_jpg[] = {
#embed "photos/snake.jpg"
};

constexpr std::array photos{lv::encoded_image<cat_jpg>, lv::encoded_image<dog_jpg>, lv::encoded_image<elephant_jpg>,
                            lv::encoded_image<snake_jpg>};
constexpr const char* names[] = {"cat.jpg", "dog.jpg", "elephant.jpg", "snake.jpg"};

constexpr int column = page_width / 2;
constexpr int photo_width = column - 2 * 20;
constexpr int photo_height = window_height - header_height - 2 * 20;
constexpr int side = 20;
constexpr int control_width = column - 2 * side;

constinit lv::state copies{lv::counter{1}};
constinit lv::state<bool> in_color{true};
constinit lv::computed color_label{&color_name, in_color};
lv::weak<lv::view<"dropdown">> file_choice;

void fewer() { copies.update(&one_fewer); }
void more() { copies.update(&one_more); }
void toggle_color() { in_color = !in_color.get(); }

void start() {
  if (file_choice) start_job(names[file_choice.get().get<"selected">()]);
}

constexpr auto title_font = type(tok.text.title);
constexpr auto label_font = type(tok.text.label);
constexpr auto body_font = type(tok.text.body);

constexpr int y_file = header_height + side;
constexpr int y_copies = y_file + choice_height + side;
constexpr int y_color = y_copies + control_height + side;
constexpr int y_start = y_color + 22 + 4 + 50 + side;

constexpr auto ui = lv::group({.size = {100 * pct, 100 * pct},
                               .style = {.bg_color = tok.window.background, .bg_opa = tok.solid}})[
    back_header("USB", "Choose Settings", title_font, body_font),
    lv::image({.id = "photo", .size = {photo_width * px, photo_height * px}, .src = photos[0],
               .style = {.radius = tok.window.radius}, .x = side * px, .y = y_file * px}),
    lv::group({.x = (column + side) * px, .y = y_file * px})[
        choice("Select File:", "cat.jpg\ndog.jpg\nelephant.jpg\nsnake.jpg", label_font, control_width, "file")],
    lv::group({.x = (column + side) * px, .y = y_copies * px})[
        spin_box("Copies", copies, &fewer, &more, body_font)],
    lv::label({.style = {.text_color = tok.label.primary, .text_font = label_font}, .text = "Color",
               .x = (column + side) * px, .y = y_color * px}),
    lv::group({.x = (column + side) * px, .y = (y_color + 26) * px})[
        color_button(color_label, control_width, 50, &toggle_color, body_font)],
    lv::group({.x = (column + side) * px, .y = (y_start + 26) * px})[
        wide_button("Start printing", control_width, 60, &start, tok.button.primary, tok.button.primary, label_font)]];

// The photo follows the file that is chosen.
lv::task<> show_chosen(lv::view<"image"> photo, lv::view<"dropdown"> file) {
  for (;;) {
    co_await file.next().value_changed;
    photo.set<"src">(photos[file.get<"selected">()]);
  }
}

}  // namespace

lv::owned<lv::obj> mount_usb(lv::obj parent) {
  auto built = lv::mount<ui>(parent);
  if (!built) return {};
  file_choice = lv::weak<lv::view<"dropdown">>{built->get<"file">()};
  lv::spawn(show_chosen(built->get<"photo">(), built->get<"file">()), built->root());
  return lv::owned<lv::obj>{built->detach()};
}

}  // namespace printer
