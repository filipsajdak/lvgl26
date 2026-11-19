// A file that includes <lv/lv.hpp> and names nothing. It is compiled under the operation
// limit lvgl26_include_only_ops (tests/CMakeLists.txt): a header that scans LVGL's names or the
// global namespace where it is read, in every file, does far more in one evaluation than that.
#include <lv/lv.hpp>

int main() { return 0; }
