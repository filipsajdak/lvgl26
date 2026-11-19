#pragma once

// editor-check: skip, it needs SDL2, which the window examples find in CMake
//
// A desktop window for an lvgl26 program, written with SDL2. It joins the application's
// loop, shows an lv::frame as the panel of a display, and feeds its mouse to an lv::pointer.
// The application does not know SDL; it gets the same lv::display and lv::pointer as on a
// device or in a browser page.
//
//   constinit lv::frame<320, 240> frame;
//   lv::application app{lv::host};
//   platform::sdl_window window{app, "Speed", frame};   // joins the application's loop
//   lv::display display{app, frame, window};            // the window shows the frame
//   lv::pointer mouse{display, window};                 // the window's mouse feeds it
//   ...
//   return app.run();                                   // closing the window ends it
//
// A close request ends run() unless the program decides otherwise: window.on_close(&decide),
// where decide() returns platform::close::quit, ::ignore or ::hide. window.hide() and
// window.show() keep the display, its trees and their state; a hidden window gets no input,
// its display draws nothing, and run() goes on.

#include <lv/application.hpp>
#include <lv/display.hpp>
#include <lv/pointer.hpp>
#include <lv/units.hpp>

#include <SDL.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>

namespace platform {

// What a close request of the window does.
enum class close { quit, ignore, hide };

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
  lv::application& app_;
  const lv::frame<Width, Height>& frame_;
  SDL_Window* window_ = nullptr;
  SDL_Renderer* renderer_ = nullptr;
  SDL_Texture* texture_ = nullptr;
  lv::display* display_ = nullptr;
  lv::pointer_feed mouse_{};
  close (*on_close_)() = nullptr;
  lv::in_loop<sdl_window> loop_;  // last: the window is complete when it joins

  static constexpr std::size_t pixel_bytes = lv::frame<Width, Height>::stride / static_cast<std::size_t>(Width);

  void close_requested() {
    switch (on_close_ != nullptr ? on_close_() : close::quit) {
      case close::quit: app_.quit(); break;
      case close::hide: hide(); break;
      case close::ignore: break;
    }
  }

  void visible(bool now) {
    if (display_ != nullptr) display_->set_visible(now);
  }

  void window_event(const SDL_WindowEvent& e) {
    if (e.event == SDL_WINDOWEVENT_CLOSE) close_requested();
    if (e.event == SDL_WINDOWEVENT_HIDDEN || e.event == SDL_WINDOWEVENT_MINIMIZED) visible(false);
    if (e.event == SDL_WINDOWEVENT_SHOWN || e.event == SDL_WINDOWEVENT_RESTORED) visible(true);
  }

  void mouse_event(const SDL_Event& e) {
    if (!mouse_) return;
    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT)
      static_cast<void>(mouse_({e.button.x, e.button.y}, lv::pressed));
    if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT)
      static_cast<void>(mouse_({e.button.x, e.button.y}, lv::released));
    if (e.type == SDL_MOUSEMOTION)
      static_cast<void>(mouse_({e.motion.x, e.motion.y}, (e.motion.state & SDL_BUTTON_LMASK) != 0 ? lv::pressed : lv::released));
  }

 public:
  sdl_window(lv::application& app, const char* title, const lv::frame<Width, Height>& frame)
      : app_(app), frame_(frame), loop_(app, *this) {
    SDL_Init(SDL_INIT_VIDEO);
    window_ = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, Width, Height,
                               SDL_WINDOW_RESIZABLE);
    renderer_ = SDL_CreateRenderer(window_, -1, 0);
    SDL_RenderSetLogicalSize(renderer_, Width, Height);  // mouse positions come in pixels of the frame
    texture_ = SDL_CreateTexture(renderer_, texture_format<pixel_bytes>(), SDL_TEXTUREACCESS_STREAMING, Width, Height);
  }
  sdl_window(lv::application&&, const char*, const lv::frame<Width, Height>&) = delete;
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

  // What a close request does, instead of ending run().
  void on_close(close (*decide)()) { on_close_ = decide; }

  // Hides the window, or shows it again. Its display, trees and state stay.
  void hide() {
    SDL_HideWindow(window_);
    visible(false);
  }
  void show() {
    SDL_ShowWindow(window_);
    visible(true);
  }
  bool shown() const { return (SDL_GetWindowFlags(window_) & SDL_WINDOW_HIDDEN) == 0; }

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

  // The display and the pointer that the window serves.
  void attach(lv::display& d) { display_ = &d; }
  void detach(lv::display&) { display_ = nullptr; }
  void attach(lv::pointer_feed feed) { mouse_ = feed; }
  void detach(lv::pointer_feed feed) {
    if (mouse_ == feed) mouse_ = {};
  }

  // The application's loop: SDL's events before each step.
  void poll() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_QUIT) close_requested();
      if (e.type == SDL_WINDOWEVENT) window_event(e.window);
      mouse_event(e);
    }
  }

  // Waits until the application's next step is due, or an event comes.
  void wait(lv::duration due) const { SDL_WaitEventTimeout(nullptr, static_cast<int>(std::min(due.ms, longest_wait_ms))); }
};

}  // namespace platform
