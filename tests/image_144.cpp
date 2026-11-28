// A photo of 144 x 144 RGBA pixels, decoded by the compiler within GCC's default operation limit
// under every preset, the sanitizers' included, whose flags make constant evaluation dearer.

#include <lv/image.hpp>

constexpr unsigned char cup_png[] = {
#embed "images/cup_144.png"
};

int main() {
  const auto* dsc = lv::image_file<cup_png>;
  return dsc->header.w == 144 && dsc->header.h == 144 ? 0 : 1;
}
