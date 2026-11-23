#pragma once

// The bouncing ball for the page: scene.hpp as it was, and the three things platform.cpp needs.
#include "scene.hpp"

namespace app {

inline constexpr std::int32_t width  = screen_width;
inline constexpr std::int32_t height = screen_height;
inline constexpr const auto& ui      = scene;

// The tasks of the scene, on the mounted tree.
template <class Mounted>
void start(const Mounted& root) {
  start_scene(root->root());
}

// No state is shown. The motion is the test.
inline std::int32_t state(std::int32_t) { return -1; }

}  // namespace app
