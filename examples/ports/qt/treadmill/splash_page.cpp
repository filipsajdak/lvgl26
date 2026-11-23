// The splash screen: an orange backdrop, the slogan and the wordmark sliding in.

#include <lv/image.hpp>

#include "pages.hpp"
#include "parts.hpp"

namespace treadmill {
namespace {

constexpr auto ui = lv::group(
  {
    .size = {screen_width * px, screen_height * px},
    .style = {
      .bg_color      = tok.splash.top,
      .bg_grad_color = tok.splash.bottom,
      .bg_grad_dir   = "ver",
      .bg_opa        = tok.solid,
    },
  },
  lv::image({
    .id  = "slogan",
    .src = lv::image_file<pictures::slogan>,
    .x   = slogan_from * px,
    .y   = slogan_y * px,
  }),
  lv::image({
    .id  = "wordmark",
    .src = lv::image_file<pictures::wordmark>,
    .x   = wordmark_from * px,
    .y   = wordmark_y * px,
  })
);

constexpr lv::duration slide_time =
  1000 * ms;  // frames 0 to 200 of the original's 5000 ms timeline

lv::task<> slide_in(lv::view<"image"> picture, int from, int to) {
  co_await lv::animate(picture, {.x = {from * px, to * px}}, slide_time);
}

}  // namespace

lv::owned<lv::obj> mount_splash(lv::obj parent) {
  auto built = lv::mount<ui>(parent);
  if (!built) return {};
  lv::spawn(slide_in(built->get<"slogan">(), slogan_from, slogan_to), built->get<"slogan">());
  lv::spawn(
    slide_in(built->get<"wordmark">(), wordmark_from, wordmark_to), built->get<"wordmark">()
  );
  return lv::owned<lv::obj>{built->detach()};
}

}  // namespace treadmill
