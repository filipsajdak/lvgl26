#include <lv/lv.hpp>
struct source {
  void poll() {}
};
void run(source& s) { lv::in_loop<source> loop{lv::application{}, s}; }
