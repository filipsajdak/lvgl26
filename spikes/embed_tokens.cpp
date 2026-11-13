#include <string_view>
#include <cstdint>
constexpr char raw[] = {
#embed "tokens.json"
, 0 };
constexpr std::string_view doc{raw, sizeof raw - 1};
consteval std::uint32_t color_after(std::string_view key) {
  auto p = doc.find(key); if (p == doc.npos) throw "token not found";
  p = doc.find('#', p); std::uint32_t v = 0;
  for (int i = 1; i <= 6; ++i) { char c = doc[p + i]; v = v * 16 + (c <= '9' ? c - '0' : c - 'a' + 10); }
  return v;
}
static_assert(color_after("\"primary\"") == 0x3366ff);
static_assert(color_after("\"surface\"") == 0x101418);
int main() {}
