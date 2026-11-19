// A library of the program settings_file: lvgl26_settings() compiles it with the program's file.
#include <lv/lv.hpp>

static_assert(!lv::detail::project<lv::override>.allow.threads, "the library has the settings of its program");

int settings_library_answer() { return 42; }
