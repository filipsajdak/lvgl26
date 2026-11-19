#include <lv/lv.hpp>
struct quiet {
  lv::in_loop<quiet> loop;
  explicit quiet(lv::application& app) : loop{app, *this} {}
};
void run(lv::application& app) { quiet q{app}; }
