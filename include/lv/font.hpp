#pragma once

// Fonts: the compiler reads a TrueType file and makes a constant font from it. The
// device gets glyph bitmaps in read-only memory: no heap, no rasteriser, no
// converter to run.
//
//   constexpr unsigned char barlow_ttf[] = {
//   #embed "Barlow-Medium.ttf"
//   };
//
// A font belongs to the design, so a style takes it from a typography token. A
// typeface says which file each family and weight of the tokens is:
//
//   constexpr auto type = lv::typeface({{"Barlow", 500, barlow_ttf}});
//
//   constexpr auto title = type(tok.text.title);            // family, weight and size from the token
//   lv::label({.style = {.text_font = title}})
//
// A font that no token names is a deliberate exception, and says so:
//
//   constexpr auto odd = lv::font<barlow_ttf>(18 * px);
//   lv::label({.style = {.text_font = lv::raw(odd)}})
//
// Each font holds the characters from the space to the tilde, or the ones you name:
// `lv::font<barlow_ttf>(72 * px, "0123456789:")`, `{"Barlow", 500, barlow_ttf, "0123456789:"}`.
// Kerning comes from the file. There is no hinting, so very small sizes are a little soft.
//
// Making a font costs compile time (about a second for 95 characters at 14 px, five at
// 48 px). Make each one once, in a constexpr variable, in one file.

#include <lv/detail/ttf.hpp>
#include <lv/token_types.hpp>
#include <lv/units.hpp>
#include <lv/values.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace lv {

namespace detail {

// A file embedded as an array of unsigned char.
template <class T>
concept embedded_bytes = std::is_bounded_array_v<std::remove_cvref_t<T>> &&
                         std::same_as<std::remove_cv_t<std::remove_extent_t<std::remove_cvref_t<T>>>, unsigned char>;

inline constexpr std::string_view ascii = " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";

// A number in hexadecimal, at least four digits: how a character is written (U+00B0).
consteval std::string hex(std::uint32_t v) {
  std::string out;
  for (int shift = 20; shift >= 0; shift -= 4)
    if (shift < 16 || (v >> shift) != 0) out.push_back("0123456789ABCDEF"[(v >> shift) & 15]);
  return out;
}

// The bytes that follow the first one of a UTF-8 character.
consteval std::size_t utf8_tail(unsigned char lead) { return lead < 0x80 ? 0 : lead < 0xe0 ? 1 : lead < 0xf0 ? 2 : 3; }

// The character of a UTF-8 text that starts at `i`. `i` moves past it.
consteval std::uint32_t next_character(std::string_view text, std::size_t& i) {
  const auto lead = static_cast<unsigned char>(text[i++]);
  const std::size_t tail = utf8_tail(lead);
  std::uint32_t c = tail == 0 ? lead : lead & (0x3fu >> tail);
  for (std::size_t k = 0; k < tail && i < text.size(); ++k) c = c << 6 | (static_cast<unsigned char>(text[i++]) & 0x3f);
  return c;
}

// The characters of a UTF-8 text, sorted, each once.
consteval std::vector<std::uint32_t> characters_of(std::string_view text) {
  std::vector<std::uint32_t> out;
  for (std::size_t i = 0; i < text.size();) {
    const auto c = next_character(text, i);
    std::size_t at = 0;
    while (at < out.size() && out[at] < c) ++at;
    if (at == out.size() || out[at] != c) out.insert(out.begin() + static_cast<std::ptrdiff_t>(at), c);
  }
  return out;
}

struct font_parts {
  std::vector<std::uint8_t> bitmap;
  std::vector<lv_font_fmt_txt_glyph_dsc_t> glyphs{lv_font_fmt_txt_glyph_dsc_t{}};  // glyph 0 means "none"
};

// The box of a glyph in pixels: left, bottom, right and top, the baseline at 0.
struct glyph_box {
  int x0, y0, x1, y1;
};

consteval glyph_box box_of(const ttf::face& f, std::uint32_t g, float scale) {
  const std::size_t at = ttf::glyph_at(f, g);
  return {floor_of(static_cast<float>(ttf::i16(f.d, at + 2)) * scale), floor_of(static_cast<float>(ttf::i16(f.d, at + 4)) * scale),
          ceil_of(static_cast<float>(ttf::i16(f.d, at + 6)) * scale), ceil_of(static_cast<float>(ttf::i16(f.d, at + 8)) * scale)};
}

// Draws glyph `g` and adds its pixels: 4 bits each, rows without a gap, the high
// half of a byte first.
consteval void add_bitmap(font_parts& to, const ttf::face& f, std::uint32_t g, float scale, glyph_box box) {
  ttf::contours parts;
  ttf::outline(f, g, ttf::matrix{scale, 0, 0, -scale, static_cast<float>(-box.x0), static_cast<float>(box.y1)}, parts);
  coverage drawn{box.x1 - box.x0, box.y1 - box.y0};
  for (const auto& part : parts) ttf::draw(drawn, part);
  const auto pixels = drawn.pixels(15);
  for (std::size_t i = 0; i < pixels.size(); i += 2)
    to.bitmap.push_back(static_cast<std::uint8_t>(pixels[i] << 4 | (i + 1 < pixels.size() ? pixels[i + 1] : 0)));
}

consteval void check_fits(glyph_box box, int advance) {
  if (box.x1 - box.x0 > 255 || box.y1 - box.y0 > 255 || advance > 4095 || box.x0 < -128 || box.y0 < -128)
    fail("lv: a glyph of this font is larger than LVGL's font format holds at this size (a box of 255 px). Use a smaller size.");
}

consteval void add_glyph(font_parts& to, const ttf::face& f, std::uint32_t character, float scale) {
  const auto g = ttf::glyph_of(f, character);
  if (g == 0) fail("lv: the font file has no glyph for the character U+" + hex(character) + ". Take it out of the font's characters.");
  lv_font_fmt_txt_glyph_dsc_t dsc{};
  const int advance = round_of(static_cast<float>(ttf::advance_of(f, g)) * scale * 16);
  dsc.bitmap_index = static_cast<std::uint32_t>(to.bitmap.size()) & 0xfffff;
  dsc.adv_w = static_cast<std::uint32_t>(advance) & 0xfff;
  if (!ttf::is_blank(f, g)) {
    const auto box = box_of(f, g, scale);
    check_fits(box, advance);
    add_bitmap(to, f, g, scale, box);
    dsc.box_w = static_cast<std::uint8_t>(box.x1 - box.x0), dsc.box_h = static_cast<std::uint8_t>(box.y1 - box.y0);
    dsc.ofs_x = static_cast<std::int8_t>(box.x0), dsc.ofs_y = static_cast<std::int8_t>(box.y0);
  }
  to.glyphs.push_back(dsc);
}

// Which character each glyph of the font is. One range when the characters follow
// each other, otherwise a list of their distances from the first.
consteval const lv_font_fmt_txt_cmap_t* stored_cmap(const std::vector<std::uint32_t>& characters) {
  lv_font_fmt_txt_cmap_t map{};
  map.range_start = characters.front();
  map.range_length = static_cast<std::uint16_t>(characters.back() - characters.front() + 1);
  map.glyph_id_start = 1;
  map.type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY;
  if (map.range_length != characters.size()) {
    std::vector<std::uint16_t> list;
    for (const auto c : characters) list.push_back(static_cast<std::uint16_t>(c - characters.front()));
    map.unicode_list = std::define_static_array(list).data();
    map.list_length = static_cast<std::uint16_t>(list.size());
    map.type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY;
  }
  return std::define_static_array(std::vector{map}).data();
}

consteval std::int8_t kern_value(int units, float scale) {
  const int v = round_of(static_cast<float>(units) * scale * 16);  // sixteenths of a pixel
  return static_cast<std::int8_t>(v < -128 ? -128 : v > 127 ? 127 : v);
}

// The pairs of the font's glyphs that the file moves, in the order LVGL searches them.
consteval const lv_font_fmt_txt_kern_pair_t* stored_kerning(const ttf::face& f, const std::vector<std::uint32_t>& characters,
                                                            float scale) {
  const auto subtables = ttf::pair_subtables(f);
  std::vector<std::uint32_t> glyphs;
  for (const auto c : characters) glyphs.push_back(ttf::glyph_of(f, c));
  std::vector<std::uint8_t> ids;
  std::vector<std::int8_t> values;
  for (std::size_t l = 0; l < glyphs.size() && !subtables.empty(); ++l) {
    for (std::size_t r = 0; r < glyphs.size(); ++r) {
      const auto v = kern_value(ttf::kerning(f, subtables, glyphs[l], glyphs[r]), scale);
      if (v == 0) continue;
      ids.push_back(static_cast<std::uint8_t>(l + 1)), ids.push_back(static_cast<std::uint8_t>(r + 1));
      values.push_back(v);
    }
  }
  if (values.empty()) return nullptr;
  lv_font_fmt_txt_kern_pair_t pairs{};
  pairs.glyph_ids = std::define_static_array(ids).data();
  pairs.values = std::define_static_array(values).data();
  pairs.pair_cnt = static_cast<std::uint32_t>(values.size()) & 0x3fffffff;
  pairs.glyph_ids_size = 0;  // one byte for each glyph
  return std::define_static_array(std::vector{pairs}).data();
}

consteval const lv_font_fmt_txt_dsc_t* stored_dsc(const font_parts& parts, const ttf::face& f,
                                                  const std::vector<std::uint32_t>& characters, float scale) {
  lv_font_fmt_txt_dsc_t dsc{};
  dsc.glyph_bitmap = std::define_static_array(parts.bitmap).data();
  dsc.glyph_dsc = std::define_static_array(parts.glyphs).data();
  dsc.cmaps = stored_cmap(characters);
  dsc.kern_dsc = stored_kerning(f, characters, scale);
  dsc.kern_scale = 16;
  dsc.cmap_num = 1;
  dsc.bpp = 4;
  dsc.bitmap_format = LV_FONT_FMT_TXT_PLAIN;
  return std::define_static_array(std::vector{dsc}).data();
}

// The font of `file` at `px` pixels, with static storage.
consteval const lv_font_t* make_font(ttf::bytes file, std::int32_t px, std::string_view text) {
  if (px < 4 || px > 255) fail("lv: a font size is from 4 px to 255 px.");
  const auto characters = characters_of(text.empty() ? ascii : text);
  if (characters.size() > 255) fail("lv: a font holds at most 255 characters. Make a second font for the others.");
  const ttf::face f = ttf::open(file);
  const float scale = static_cast<float>(px) / static_cast<float>(f.units);
  font_parts parts;
  for (const auto c : characters) add_glyph(parts, f, c, scale);

  const int ascent = ceil_of(static_cast<float>(ttf::i16(f.d, f.hhea + 4)) * scale);
  const int descent = floor_of(static_cast<float>(ttf::i16(f.d, f.hhea + 6)) * scale);
  lv_font_t font{};
  font.get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt;
  font.get_glyph_bitmap = lv_font_get_bitmap_fmt_txt;
  font.line_height = ascent - descent;
  font.base_line = -descent;
  font.subpx = LV_FONT_SUBPX_NONE;
  font.static_bitmap = 1;
  font.underline_position = static_cast<std::int8_t>(-(px / 12 + 1));
  font.underline_thickness = static_cast<std::int8_t>(px / 16 + 1);
  font.dsc = stored_dsc(parts, f, characters, scale);
  return std::define_static_array(std::vector{font}).data();
}

template <const auto& File>
struct font_file {
  // The font at `size`, with `characters` (UTF-8), or with the space to the tilde.
  consteval const lv_font_t* operator()(length size, std::string_view characters = {}) const {
    return make_font(File, size.raw(), characters);
  }
};

}  // namespace detail

// The embedded TrueType file `File`: lv::font<barlow_ttf>(18 * px) is a font of it.
// A style takes such a font only as lv::raw(...): a font of the design comes from a
// typography token, through lv::typeface.
template <const auto& File>
  requires detail::embedded_bytes<decltype(File)>
inline constexpr detail::font_file<File> font{};

// One file of a typeface: the family and weight it is, as the design tokens name them.
struct face_file {
  std::string_view family;
  std::int32_t weight;
  std::span<const unsigned char> file;
  std::string_view characters;  // UTF-8; empty: the space to the tilde

  template <std::size_t N>
  constexpr face_file(std::string_view family_name, std::int32_t font_weight, const unsigned char (&bytes)[N],
                      std::string_view only = {})
      : family(family_name), weight(font_weight), file(bytes), characters(only) {}
  constexpr face_file() : weight(0) {}
};

// The font files of a design: lv::typeface({{"Barlow", 500, barlow_ttf}, ...}).
// Called with a typography token it gives the font the token asks for.
class typeface {
 public:
  consteval typeface(std::initializer_list<face_file> files) {
    if (files.size() > files_.size()) detail::fail("lv: a typeface has at most eight files.");
    for (const auto& f : files) files_[count_++] = f;
  }

  consteval token<const lv_font_t*> operator()(const token<typography_token>& t) const {
    for (std::size_t i = 0; i < count_; ++i)
      if (files_[i].family == t->font_family && files_[i].weight == t->font_weight)
        return {detail::make_font(files_[i].file, t->font_size.raw(), files_[i].characters), detail::from_design_tokens{}};
    detail::fail("lv: the typeface has no file for the family '" + detail::str(t->font_family) + "' at weight " +
                 detail::decimal(static_cast<std::size_t>(t->font_weight)) + ". Add it: lv::typeface({{\"" + detail::str(t->font_family) + "\", " +
                 detail::decimal(static_cast<std::size_t>(t->font_weight)) + ", its_ttf}}).");
  }

 private:
  std::array<face_file, 8> files_{};
  std::size_t count_ = 0;
};

}  // namespace lv
