// The fonts lv::font makes are the bytes that one constant evaluation of the whole font
// made. This test compares a digest of every table of a font with the digest of that font,
// so that a change in how the compiler draws a font cannot change a pixel unnoticed.
//
//   font_bytes            checks the digests
//   font_bytes --print    prints them

#include <lv/font.hpp>
#include <lv/lv.hpp>

#include "check.hpp"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string_view>

namespace {

using namespace lv::units;

constexpr unsigned char barlow_ttf[] = {
#embed "fonts/Barlow-Medium.ttf"
};
constexpr unsigned char condensed_ttf[] = {
#embed "fonts/BarlowCondensed-SemiBold.ttf"
};
constexpr unsigned char ubuntu_ttf[] = {
#embed "examples/libs/tiny_ttf/Ubuntu-Medium.ttf"
};
constexpr unsigned char montserrat_ttf[] = {
#embed "scripts/generators/built_in_font/Montserrat-Medium.ttf"
};

// FNV-1a, 64 bits: its offset basis and its prime.
constexpr std::uint64_t fnv_basis = 0xcbf29ce484222325;
constexpr std::uint64_t fnv_prime = 0x100000001b3;

struct digest {
  std::uint64_t value = fnv_basis;

  void add(std::uint64_t v) {
    for (std::size_t byte = 0; byte < sizeof v; ++byte) value = (value ^ ((v >> (8 * byte)) & 0xff)) * fnv_prime;
  }
};

void add_glyphs(digest& d, const lv_font_fmt_txt_dsc_t* dsc, std::uint32_t count) {
  std::uint32_t end = 0;
  for (std::uint32_t i = 0; i <= count; ++i) {
    const auto& g = dsc->glyph_dsc[i];
    d.add(g.bitmap_index), d.add(g.adv_w), d.add(g.box_w), d.add(g.box_h);
    d.add(static_cast<std::uint64_t>(g.ofs_x)), d.add(static_cast<std::uint64_t>(g.ofs_y));
    const std::uint32_t bytes = (static_cast<std::uint32_t>(g.box_w) * g.box_h + 1) / 2;
    if (g.bitmap_index + bytes > end) end = g.bitmap_index + bytes;
  }
  for (std::uint32_t i = 0; i < end; ++i) d.add(dsc->glyph_bitmap[i]);
}

void add_cmap(digest& d, const lv_font_fmt_txt_cmap_t& map) {
  for (const std::uint32_t v : {map.range_start, std::uint32_t{map.range_length}, std::uint32_t{map.glyph_id_start},
                                std::uint32_t{map.list_length}, static_cast<std::uint32_t>(map.type)})
    d.add(v);
  for (std::uint32_t i = 0; map.unicode_list != nullptr && i < map.list_length; ++i) d.add(map.unicode_list[i]);
}

void add_kerning(digest& d, const void* kern) {
  const auto* pairs = static_cast<const lv_font_fmt_txt_kern_pair_t*>(kern);
  if (pairs == nullptr) return;
  d.add(pairs->pair_cnt);
  const auto* ids = static_cast<const std::uint8_t*>(pairs->glyph_ids);
  for (std::uint32_t i = 0; i < pairs->pair_cnt; ++i) d.add(ids[2 * i]), d.add(ids[2 * i + 1]), d.add(static_cast<std::uint64_t>(pairs->values[i]));
}

std::uint64_t digest_of(const lv_font_t* font) {
  digest d;
  for (const auto v : {font->line_height, font->base_line}) d.add(static_cast<std::uint64_t>(v));
  d.add(static_cast<std::uint64_t>(font->underline_position)), d.add(static_cast<std::uint64_t>(font->underline_thickness));
  const auto* dsc = static_cast<const lv_font_fmt_txt_dsc_t*>(font->dsc);
  const auto& map = dsc->cmaps[0];
  add_cmap(d, map);
  add_glyphs(d, dsc, map.list_length != 0 ? map.list_length : map.range_length);
  add_kerning(d, dsc->kern_dsc);
  return d.value;
}

struct case_ {
  std::string_view name;
  const lv_font_t* font;
  std::uint64_t made_in_one_evaluation;  // the digest of the font that one evaluation made
};

constexpr case_ cases[] = {
    {"Barlow Medium, 32 px, the space to the tilde", lv::font<barlow_ttf>(32 * px), 0xade9bd71bae2445e},
    {"Barlow Medium, 16 px, three characters not in a row", lv::font<barlow_ttf>(16 * px, "\xC2\xB0""C9"), 0x76cd30964b54cd7a},
    {"Barlow Condensed SemiBold, 64 px, digits and colon", lv::font<condensed_ttf>(64 * px, "0123456789:"), 0x4bf419ea87089ae3},
    {"Barlow Condensed SemiBold, 104 px, numerals", lv::font<condensed_ttf>(104 * px, "0123456789.:-%\xC2\xB0 "), 0x02a72cf8d1aacd18},
    {"Barlow Condensed SemiBold, 208 px, numerals", lv::font<condensed_ttf>(208 * px, "0123456789.:-%\xC2\xB0 "), 0xd940d46534a8ca9e},
    {"Ubuntu Medium, 24 px, four-byte offsets", lv::font<ubuntu_ttf>(24 * px, "AVTo."), 0x3ed34a12a85ee133},
    {"Montserrat Medium, 24 px, kerning in lookups of type 9", lv::font<montserrat_ttf>(24 * px, "AVTo."), 0x6d085eae620472e8},
};

}  // namespace

int main(int argc, char** argv) {
  const bool print = argc == 2 && std::strcmp(argv[1], "--print") == 0;
  for (const auto& c : cases) {
    const auto value = digest_of(c.font);
    if (print) std::printf("0x%016llx  %.*s\n", static_cast<unsigned long long>(value), static_cast<int>(c.name.size()), c.name.data());
    else check(value == c.made_in_one_evaluation);
  }
  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
