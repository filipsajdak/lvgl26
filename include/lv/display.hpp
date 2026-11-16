#pragma once

// A display: the memory LVGL draws into, and how the pixels reach a panel.
//
//   constinit lv::frame<320, 240> frame;          // memory for the whole screen
//
//   lv::application app;
//   lv::display display{app, frame};              // no panel: the picture stays in the frame
//   lv::display display{app, frame, &changed};    // void changed(lv::area)
//
//   constinit lv::band<320, 240, 24> band;        // memory for 24 lines of a 320 x 240 screen
//   lv::display display{app, band, &send};        // void send(lv::area, std::span<const std::byte>)
//
// A frame holds the current picture at all times. LVGL draws each change in place and
// then names the area that changed; a panel with its own memory copies that area from
// the frame. A band saves memory: LVGL draws the screen piece by piece and hands over
// each piece, which must be sent before the call returns.
//
// The panel is a function, or a named object that can be called the same way. The
// display keeps a pointer to the object, so a temporary is rejected. A transfer that
// finishes after the call returns is not supported yet.
//
// display.screen() is the screen that is shown: mount a tree on it. The display is
// deleted when the object ends, so declare it after the application.

#include <lv/application.hpp>
#include <lv/detail/theme_styles.hpp>
#include <lv/view.hpp>

#include <lvgl/lvgl.h>

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>

namespace lv {

// A rectangle of the screen, in pixels.
struct area {
  std::int32_t x = 0;
  std::int32_t y = 0;
  std::int32_t width = 0;
  std::int32_t height = 0;
  friend constexpr bool operator==(area, area) = default;
};

namespace detail {

// The bytes of one line of `Width` pixels in the color format LVGL is built for.
template <std::int32_t Width>
inline constexpr std::size_t line_bytes = LV_DRAW_BUF_STRIDE(Width, LV_COLOR_FORMAT_NATIVE);

// The bytes LVGL asks for to draw `Lines` lines of `Width` pixels.
template <std::int32_t Width, std::int32_t Lines>
inline constexpr std::size_t draw_bytes = LV_DRAW_BUF_SIZE(Width, Lines, LV_COLOR_FORMAT_NATIVE);

}  // namespace detail

// Memory for a whole screen of Width x Height pixels.
template <std::int32_t Width, std::int32_t Height>
struct frame {
  static_assert(Width > 0 && Height > 0, "lv::frame<Width, Height>: both must be greater than 0");

  static constexpr std::int32_t width = Width;
  static constexpr std::int32_t height = Height;
  static constexpr std::size_t stride = detail::line_bytes<Width>;  // bytes from one line to the next

  // The picture: `height` lines of `stride` bytes, in LVGL's color format.
  std::span<const std::byte> bytes() const { return {memory, stride * static_cast<std::size_t>(Height)}; }

  alignas(LV_DRAW_BUF_ALIGN) std::byte memory[detail::draw_bytes<Width, Height>]{};
};

// Memory for `Lines` lines of a screen of Width x Height pixels.
template <std::int32_t Width, std::int32_t Height, std::int32_t Lines>
struct band {
  static_assert(Width > 0 && Height > 0, "lv::band<Width, Height, Lines>: the screen must be larger than 0");
  static_assert(Lines >= 1 && Lines <= Height,
                "lv::band<Width, Height, Lines>: Lines is the height of the band, from 1 to Height");

  static constexpr std::int32_t width = Width;
  static constexpr std::int32_t height = Height;
  static constexpr std::int32_t lines = Lines;

  alignas(LV_DRAW_BUF_ALIGN) std::byte memory[detail::draw_bytes<Width, Lines>]{};
};

// An object that is told which part of a frame changed.
template <class P>
concept frame_panel = std::is_class_v<P> && std::invocable<P&, area>;

// An object that is given a piece of the screen.
template <class P>
concept band_panel = std::is_class_v<P> && std::invocable<P&, area, std::span<const std::byte>>;

class display {
 public:
  template <std::int32_t W, std::int32_t H>
  display(application&, frame<W, H>& memory, theme_styles look = {})
      : display(W, H, memory.memory, LV_DISPLAY_RENDER_MODE_DIRECT) {
    use(look);
  }

  template <std::int32_t W, std::int32_t H>
  display(application& app, frame<W, H>& memory, void (*changed)(area), theme_styles look = {})
      : display(app, memory, look) {
    function_ = reinterpret_cast<void (*)()>(changed);
    call_ = [](const display& self, area where, std::span<const std::byte>) {
      reinterpret_cast<void (*)(area)>(self.function_)(where);
    };
  }

  template <std::int32_t W, std::int32_t H, frame_panel P>
  display(application& app, frame<W, H>& memory, P& panel, theme_styles look = {}) : display(app, memory, look) {
    object_ = &panel;
    call_ = [](const display& self, area where, std::span<const std::byte>) {
      (*static_cast<P*>(self.object_))(where);
    };
  }

  template <std::int32_t W, std::int32_t H, class P>
  display(application&, frame<W, H>&, P&&) = delete(
      "the panel of a frame is a function void(lv::area), or a named object that is called that "
      "way: it is told which area changed, and the pixels are in the frame. The display keeps a "
      "pointer to the object, so it cannot be a temporary");

  template <std::int32_t W, std::int32_t H, std::int32_t L>
  display(application&, band<W, H, L>& memory, void (*send)(area, std::span<const std::byte>),
          theme_styles look = {})
      : display(W, H, memory.memory, LV_DISPLAY_RENDER_MODE_PARTIAL) {
    use(look);
    function_ = reinterpret_cast<void (*)()>(send);
    call_ = [](const display& self, area where, std::span<const std::byte> pixels) {
      reinterpret_cast<void (*)(area, std::span<const std::byte>)>(self.function_)(where, pixels);
    };
  }

  template <std::int32_t W, std::int32_t H, std::int32_t L, band_panel P>
  display(application&, band<W, H, L>& memory, P& panel, theme_styles look = {})
      : display(W, H, memory.memory, LV_DISPLAY_RENDER_MODE_PARTIAL) {
    use(look);
    object_ = &panel;
    call_ = [](const display& self, area where, std::span<const std::byte> pixels) {
      (*static_cast<P*>(self.object_))(where, pixels);
    };
  }

  template <std::int32_t W, std::int32_t H, std::int32_t L, class P>
  display(application&, band<W, H, L>&, P&&) = delete(
      "the panel of a band is a function void(lv::area, std::span<const std::byte>), or a named "
      "object that is called that way: it is given each piece of the screen. The display keeps a "
      "pointer to the object, so it cannot be a temporary");

  display(const display&) = delete;
  display& operator=(const display&) = delete;

  ~display() {
    lv_display_delete(raw_);
    if (theme_ != nullptr) lv_theme_delete(theme_);
  }

  // The screen that is shown.
  obj screen() const { return obj{lv_display_get_screen_active(raw_)}; }

  std::int32_t width() const { return lv_display_get_horizontal_resolution(raw_); }
  std::int32_t height() const { return lv_display_get_vertical_resolution(raw_); }

  // The LVGL display, for code that still calls the C API.
  lv_display_t* raw() const { return raw_; }

 private:
  display(std::int32_t width, std::int32_t height, std::span<std::byte> memory, lv_display_render_mode_t mode)
      : raw_(lv_display_create(width, height)) {
    contract_assert(raw_ != nullptr);  // LVGL's heap is too small for a display
    lv_display_set_driver_data(raw_, this);
    lv_display_set_buffers(raw_, memory.data(), nullptr, static_cast<std::uint32_t>(memory.size()), mode);
    lv_display_set_flush_cb(raw_, &drawn);
  }

  // Puts `look` after the theme the display has, and styles the screen with it. The
  // screen is still empty: a display is given its theme when it is made.
  void use(theme_styles look) {
    if (look.empty()) return;
    theme_ = lv_theme_create();
    contract_assert(theme_ != nullptr);  // LVGL's heap is too small for a theme
    lv_theme_set_parent(theme_, lv_display_get_theme(raw_));
    lv_theme_set_apply_cb(theme_, &detail::apply_theme);
    lv_theme_set_user_data(theme_, const_cast<detail::theme_entry*>(detail::theme_access::first(look)));
    lv_display_set_theme(raw_, theme_);
    // LVGL styles the bottom layer there, not the screen that is shown.
    lv_theme_apply(lv_display_get_screen_active(raw_));
  }

  // LVGL has drawn `where`. In a frame `pixels` is the frame; in a band it is the piece.
  static void drawn(lv_display_t* raw, const lv_area_t* where, std::uint8_t* pixels) {
    const auto& self = *static_cast<const display*>(lv_display_get_driver_data(raw));
    if (self.call_ != nullptr) self.call_(self, self.area_of(*where), self.piece(*where, pixels));
    lv_display_flush_ready(raw);
  }

  area area_of(const lv_area_t& where) const {
    return {where.x1, where.y1, lv_area_get_width(&where), lv_area_get_height(&where)};
  }

  std::span<const std::byte> piece(const lv_area_t& where, const std::uint8_t* pixels) const {
    const std::uint32_t stride =
        lv_draw_buf_width_to_stride(static_cast<std::uint32_t>(lv_area_get_width(&where)),
                                    lv_display_get_color_format(raw_));
    return {reinterpret_cast<const std::byte*>(pixels),
            static_cast<std::size_t>(stride) * static_cast<std::size_t>(lv_area_get_height(&where))};
  }

  lv_display_t* raw_;
  lv_theme_t* theme_ = nullptr;
  void* object_ = nullptr;
  void (*function_)() = nullptr;
  void (*call_)(const display&, area, std::span<const std::byte>) = nullptr;
};

}  // namespace lv
