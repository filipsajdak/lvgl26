// The main screen in the "dark" look.

#include "main_page.hpp"

namespace treadmill {
namespace {

constexpr auto ui =
  main_screen<tok.theme.dark, pictures::incline_light, pictures::speed_light, look::dark>();

}  // namespace

lv::owned<lv::obj> mount_main_dark(lv::obj parent) {
  auto built = lv::mount<ui>(parent);
  if (!built) return {};
  lv::spawn(follow_runner(built->get<"runner">()), built->get<"runner">());
  return lv::owned<lv::obj>{built->detach()};
}

}  // namespace treadmill
