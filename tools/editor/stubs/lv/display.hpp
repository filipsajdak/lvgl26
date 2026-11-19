#pragma once

// Editor stub of lv/display.hpp: the memory LVGL draws into, and the way to a panel.

#include <lv/application.hpp>
#include <lv/detail/theme_styles.hpp>
#include <lv/view.hpp>

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>

namespace lv {

/// A rectangle of the screen, in pixels.
struct area {
  std::int32_t x = 0;
  std::int32_t y = 0;
  std::int32_t width = 0;
  std::int32_t height = 0;
  friend constexpr bool operator==(area, area) = default;
};

/// A point of the screen, in pixels from its top left corner.
struct point {
  std::int32_t x = 0;
  std::int32_t y = 0;
  friend constexpr bool operator==(point, point) = default;
};

/// Memory for a whole screen of Width x Height pixels: constinit lv::frame<320, 240> frame;
template <std::int32_t Width, std::int32_t Height>
struct frame {
  static_assert(Width > 0 && Height > 0, "lv::frame<Width, Height>: both must be greater than 0");

  static constexpr std::int32_t width = Width;
  static constexpr std::int32_t height = Height;
  /// Bytes from one line to the next.
  static constexpr std::size_t stride = LV_DRAW_BUF_STRIDE(Width, LV_COLOR_FORMAT_NATIVE);

  /// The picture: `height` lines of `stride` bytes, in LVGL's color format.
  std::span<const std::byte> bytes() const { __builtin_unreachable(); }

  std::byte memory[1]{};
};

/// Memory for `Lines` lines of a screen of Width x Height pixels.
template <std::int32_t Width, std::int32_t Height, std::int32_t Lines>
struct band {
  static_assert(Width > 0 && Height > 0, "lv::band<Width, Height, Lines>: the screen must be larger than 0");
  static_assert(Lines >= 1 && Lines <= Height,
                "lv::band<Width, Height, Lines>: Lines is the height of the band, from 1 to Height");

  static constexpr std::int32_t width = Width;
  static constexpr std::int32_t height = Height;
  static constexpr std::int32_t lines = Lines;

  std::byte memory[1]{};
};

/// An object that is told which part of a frame changed.
template <class P>
concept frame_panel = std::is_class_v<P> && std::invocable<P&, area>;

/// An object that is given a piece of the screen.
template <class P>
concept band_panel = std::is_class_v<P> && std::invocable<P&, area, std::span<const std::byte>>;

class display;

/// A panel told which display shows it: attach(display&) and detach(display&).
template <class P>
concept attached_panel = requires(P& p, display& d) {
  p.attach(d);
  p.detach(d);
};

/// A panel that can hide and show itself, such as a window.
template <class P>
concept hideable_panel = requires(P& p) {
  p.hide();
  p.show();
};

/// A display. Declare it after the application; mount a tree on its screen().
class display {
 public:
  /// No panel: the picture stays in the frame. `look` is a theme, such as lv::signal::dark.
  template <std::int32_t W, std::int32_t H>
  display(application&, frame<W, H>& memory, theme_styles look = {}) {}

  /// A panel function: void changed(lv::area) is told which part of the frame is new.
  template <std::int32_t W, std::int32_t H>
  display(application& app, frame<W, H>& memory, void (*changed)(area), theme_styles look = {}) {}

  /// A panel object, called as changed(lv::area). The display keeps a pointer to it.
  template <std::int32_t W, std::int32_t H, frame_panel P>
  display(application& app, frame<W, H>& memory, P& panel, theme_styles look = {}) {}

  /// A band: void send(lv::area, std::span<const std::byte>) gets each piece of the screen.
  template <std::int32_t W, std::int32_t H, std::int32_t L>
  display(application&, band<W, H, L>& memory, void (*send)(area, std::span<const std::byte>),
          theme_styles look = {}) {}

  /// A band with a panel object. The display keeps a pointer to it.
  template <std::int32_t W, std::int32_t H, std::int32_t L, band_panel P>
  display(application&, band<W, H, L>& memory, P& panel, theme_styles look = {}) {}

  display(const display&) = delete;
  display& operator=(const display&) = delete;
  ~display();

  /// The screen that is shown: lv::mount<ui>(display.screen()).
  obj screen() const;
  std::int32_t width() const;
  std::int32_t height() const;
  /// The LVGL display, for code that still calls the C API.
  lv_display_t* raw() const;
  /// Whether anyone can see the display. While it is not, LVGL draws nothing.
  bool visible() const noexcept;
  /// What the platform says: its window was hidden or minimised (false), shown or restored (true).
  void set_visible(bool now);
  /// Asks the panel to hide or show itself; a panel that cannot only changes visible().
  void hide();
  void show();
};

/// The display a widget is on. The widget must be on an lv::display.
display& display_of(obj widget);

}  // namespace lv
