// A look with a size in percent, beside a drawn font and a picture: the objects that a scan of the global namespace
// would walk. A percentage in a look must not start such a scan (docs/research/look-percent.md is this note).
#include <lv/font.hpp>
#include <lv/image.hpp>
#include <lv/look.hpp>
#include <lv/lv.hpp>

using namespace lv::units;

namespace {

constexpr char tokens_json[] = {
#embed "look_tokens.json"
    , 0};
constexpr unsigned char barlow[] = {
#embed "fonts/Barlow-Medium.ttf"
};
constexpr unsigned char noise_png[] = {
#embed "images/noise.png"
};
constexpr auto tok = lv::tokens<tokens_json>;

}  // namespace

template <>
inline constexpr lv::design_of<decltype(tok)> lv::design<lv::override>{
    .tokens = tok, .type = lv::typeface({{"Barlow", 500, barlow}})};

namespace {

constexpr auto photo = lv::image_file<noise_png>;
constexpr auto label_font = lv::font<barlow>(16 * px);

// "pct99" is a width of 99%: the percentage entry of a look.
constexpr auto ui = lv::group({.id = "page", .size = {320 * px, 240 * px}},
                              lv::box({.id = "wide", .style = lv::look<"pct99">}),
                              lv::image({.id = "photo", .src = photo}),
                              lv::label({.text = "Hello", .style = {.text_font = lv::raw(label_font)}}));

constinit lv::frame<320, 240> frame;

}  // namespace

int main() {
  lv::application app;
  lv::display display{app, frame};
  auto root = lv::mount<ui>(display.screen());
  return root ? 0 : 1;
}
