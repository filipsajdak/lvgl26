#include <cstdio>
import lv3;
int main() { auto l = lv::make_label(); l.set<"text">("table in module"); std::printf("%s\n", l.text()); }
