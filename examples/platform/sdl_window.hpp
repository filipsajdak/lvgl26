#pragma once

// editor-check: skip, it needs SDL2, which the window examples find in CMake
//
// A desktop window for an lvgl26 program, written with SDL2: the window shows an lv::frame,
// and the mouse feeds an lv::pointer. The application does not know SDL; it gets the same
// lv::display and lv::pointer as on a device or in a browser page.
//
//   constinit lv::frame<320, 240> frame;
//   lv::application app{lv::host};
//   platform::sdl_window window{"Speed", frame};
//   lv::display display{app, frame, window};   // the window is the panel of the frame
//   lv::pointer mouse{display};
//   ...
//   while (window.feed(mouse)) window.wait(app.step());   // until the window is closed

#include <lv/display.hpp>
#include <lv/pointer.hpp>
#include <lv/units.hpp>

#include <SDL.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>

namespace platform {

// The SDL texture format that holds the frame's bytes as they are, from the bytes of a pixel.
template <std::size_t BytesPerPixel>
consteval std::uint32_t texture_format() {
  static_assert(BytesPerPixel == 2 || BytesPerPixel == 3 || BytesPerPixel == 4,
                "the window shows a frame of 16, 24 or 32 bits a pixel");
  if constexpr (BytesPerPixel == 2) return SDL_PIXELFORMAT_RGB565;
  if constexpr (BytesPerPixel == 3) return SDL_PIXELFORMAT_BGR24;
  return SDL_PIXELFORMAT_ARGB8888;
}

// The longest wait between two steps: a mouse event waits at most this long.
inline constexpr std::uint32_t longest_wait_ms = 10;

template <std::int32_t Width, std::int32_t Height>
class sdl_window {
  const lv::frame<Width, Height>& frame_;
  SDL_Window* window_ = nullptr;
  SDL_Renderer* renderer_ = nullptr;
  SDL_Texture* texture_ = nullptr;

  static constexpr std::size_t pixel_bytes = lv::frame<Width, Height>::stride / static_cast<std::size_t>(Width);

 public:
  sdl_window(const char* title, const lv::frame<Width, Height>& frame) : frame_(frame) {
    SDL_Init(SDL_INIT_VIDEO);
    window_ = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, Width, Height,
                               SDL_WINDOW_RESIZABLE);
    renderer_ = SDL_CreateRenderer(window_, -1, 0);
    SDL_RenderSetLogicalSize(renderer_, Width, Height);  // mouse positions come in pixels of the frame
    texture_ = SDL_CreateTexture(renderer_, texture_format<pixel_bytes>(), SDL_TEXTUREACCESS_STREAMING, Width, Height);
  }
  sdl_window(const sdl_window&) = delete;
  sdl_window& operator=(const sdl_window&) = delete;
  ~sdl_window() {
    SDL_DestroyTexture(texture_);
    SDL_DestroyRenderer(renderer_);
    SDL_DestroyWindow(window_);
    SDL_Quit();
  }

  // Whether the window could be opened.
  explicit operator bool() const { return texture_ != nullptr; }

  // The display calls this when LVGL has drawn `changed`: the window shows it.
  void operator()(lv::area changed) {
    const SDL_Rect r{changed.x, changed.y, changed.width, changed.height};
    const std::byte* first = frame_.bytes().data() + static_cast<std::size_t>(changed.y) * frame_.stride +
                             static_cast<std::size_t>(changed.x) * pixel_bytes;
    SDL_UpdateTexture(texture_, &r, first, static_cast<int>(frame_.stride));
    SDL_RenderClear(renderer_);
    SDL_RenderCopy(renderer_, texture_, nullptr, nullptr);
    SDL_RenderPresent(renderer_);
  }

  // Gives the mouse's events to `mouse`. False once the window is closed.
  template <std::size_t Capacity>
  bool feed(lv::pointer<Capacity>& mouse) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_QUIT) return false;
      if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) mouse.press({e.button.x, e.button.y});
      if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT) mouse.release();
      if (e.type == SDL_MOUSEMOTION) mouse.move({e.motion.x, e.motion.y});
    }
    return true;
  }

  // Waits until the application's next step is due, or a mouse event comes.
  void wait(lv::duration due) const { SDL_WaitEventTimeout(nullptr, static_cast<int>(std::min(due.ms, longest_wait_ms))); }
};

}  // namespace platform
