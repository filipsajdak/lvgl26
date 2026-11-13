#include <cstdio>
import lv2;
int main() { auto l = lv::make_label(); l.text("resolved in module"); std::printf("%s\n", l.text()); }
