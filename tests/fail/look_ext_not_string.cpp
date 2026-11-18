// A value of the vendor data that is not a string: the reader refuses it.
#include <lv/lv.hpp>

constexpr char tokens_json[] = {
#embed "look_bad_vendor.json"
    , 0};
constexpr auto tok = lv::tokens<tokens_json>;

constexpr auto width = tok.extnotstring;

int main() {}
