#pragma once

// The clock and the sleep of an operating system, for lv::application.
// Hosted: a program for a microcontroller gives its own (see lv/application.hpp).
//
//   #include <lv/host.hpp>
//
//   lv::application app{lv::host};
//   ...
//   return app.run();        // sleeps between the steps

#include <lv/application.hpp>

#include <chrono>
#include <cstdint>
#include <thread>

namespace lv {

namespace detail {

inline duration host_clock() {
  static const auto start = std::chrono::steady_clock::now();
  const auto passed = std::chrono::steady_clock::now() - start;
  return duration{static_cast<std::uint32_t>(
      std::chrono::duration_cast<std::chrono::milliseconds>(passed).count())};
}

// The longest sleep: input that arrives meanwhile waits at most this long.
inline constexpr std::uint32_t longest_sleep_ms = 10;

inline void host_idle(duration wait) {
  const std::uint32_t ms = wait.ms < longest_sleep_ms ? wait.ms : longest_sleep_ms;
  std::this_thread::sleep_for(std::chrono::milliseconds{ms});
}

}  // namespace detail

inline constexpr application::options host{.clock = &detail::host_clock, .idle = &detail::host_idle};

}  // namespace lv
