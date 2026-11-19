// A measurement, not part of the application: counts how many times a coroutine of lvgl26 is
// resumed. Every resume goes through lv::detail::resume_task (include/lv/task.hpp). Compile
// platform.cpp with -finstrument-functions and link this file in; the page then shows the count.
#include <lv/lv.hpp>

#include <cstdint>

namespace {
constinit std::uint64_t resumes = 0;
}

extern "C" {

__attribute__((no_instrument_function)) void __cyg_profile_func_enter(void* function, void*) {
  if (function == reinterpret_cast<void*>(&lv::detail::resume_task)) ++resumes;
}

__attribute__((no_instrument_function)) void __cyg_profile_func_exit(void*, void*) {}

// The count so far, in two halves, because a WebAssembly export returns 32 bits here.
std::uint32_t web_resumes_low() { return static_cast<std::uint32_t>(resumes); }
std::uint32_t web_resumes_high() { return static_cast<std::uint32_t>(resumes >> 32); }

}  // extern "C"
