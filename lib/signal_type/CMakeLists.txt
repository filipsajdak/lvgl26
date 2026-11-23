# lvgl26::signal_type: Signal's eight fonts, each drawn once in a file of its own (lv/signal_fonts.hpp).
# A program that links it and includes lv/signal_fonts.hpp pays no compile time for the fonts, and the
# linker takes only the fonts that it names. Off by default: LVGL26_SIGNAL_TYPE.
file(GLOB signal_type_fonts CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/font_text_*.cpp)
add_library(lvgl26_signal_type STATIC ${signal_type_fonts})
add_library(lvgl26::signal_type ALIAS lvgl26_signal_type)
target_link_libraries(lvgl26_signal_type PUBLIC lvgl26::lvgl26 PRIVATE lvgl26_warnings)
# The font files lie in include/lv/signal/fonts, next to the headers that embed them.
target_compile_options(lvgl26_signal_type PRIVATE --embed-dir=${PROJECT_SOURCE_DIR}/include/lv/signal)
# The fonts that it draws, for the build's check that every font a program names has a file
# (cmake/check_fonts.cmake): a program that links the library does not define them itself.
set_target_properties(lvgl26_signal_type PROPERTIES LVGL26_FONTS
  "text.numeral-2xl@lv::signal::type_design;text.numeral-xl@lv::signal::type_design;text.numeral-l@lv::signal::type_design;text.numeral-m@lv::signal::type_design;text.title@lv::signal::type_design;text.label@lv::signal::type_design;text.body@lv::signal::type_design;text.caption@lv::signal::type_design")
