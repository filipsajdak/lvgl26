#include <cstdio>
#include <meta>
import lv;
int main() { auto l = lv::make_label(); lv::label_set<"text">(l, "from module"); std::printf("%s\n", lv::text(l)); }
