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
// Making a font costs compile time: about 3.5 seconds for 95 characters at 14 px and 7.5 at
// 48 px, half a second for sixteen characters. Make each one once, in a constexpr
// variable, in one file. Each glyph is drawn in a constant evaluation of its own, so a
// font needs no raised -fconstexpr-ops-limit: 95 characters at 255 px need at most 28
// million operations of GCC's 33.5 million.

#include <lv/detail/embedded.hpp>
#include <lv/detail/fixed_string.hpp>
#include <lv/detail/ttf.hpp>
#include <lv/token_types.hpp>
#include <lv/units.hpp>
#include <lv/values.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <memory>
#include <meta>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace lv {

namespace detail {

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

// The place of the first byte of `text` that is not part of a UTF-8 character, or its size when there is none.
consteval std::size_t invalid_utf8_at(std::string_view text) {
  for (std::size_t i = 0; i < text.size();) {
    const auto lead = static_cast<unsigned char>(text[i]);
    if ((lead >= 0x80 && lead < 0xc0) || lead >= 0xf8) return i;  // a continuation byte alone, or no lead byte
    const std::size_t tail = utf8_tail(lead);
    if (i + tail >= text.size()) return i;  // the character is cut
    for (std::size_t k = 1; k <= tail; ++k)
      if ((static_cast<unsigned char>(text[i + k]) & 0xc0) != 0x80) return i + k;
    i += tail + 1;
  }
  return text.size();
}

// The characters of Latin-1 after the default ones: U+00A0 to U+00FF, as UTF-8 (two bytes each).
consteval std::string latin1_accents() {
  std::string out;
  for (std::uint32_t c = 0xa0; c <= 0xff; ++c) {
    out.push_back(static_cast<char>(0xc0 | c >> 6));
    out.push_back(static_cast<char>(0x80 | (c & 0x3f)));
  }
  return out;
}

// The default characters and `extra`, as the text of a typeface file's `characters`.
consteval std::string ascii_plus_text(std::string_view extra, std::string_view set = "lv::ascii_plus", bool accents = false) {
  if (const std::size_t at = invalid_utf8_at(extra); at != extra.size())
    fail("lv: " + str(set) + " takes UTF-8 text, and byte " + decimal(at) + " of it is not part of a UTF-8 character.");
  std::string out = str(ascii);
  if (accents) out += latin1_accents();
  out += str(extra);
  const std::size_t base = characters_of(accents ? str(ascii) + latin1_accents() : str(ascii)).size();
  if (const std::size_t count = characters_of(out).size(); count > 255)
    fail("lv: a font holds at most 255 characters, and the " + decimal(base) + " of " + (accents ? "lv::latin1" : "the default set") +
         " and the " + decimal(count - base) + " you add are " + decimal(count) + ". Make a second font for the others.");
  return out;
}

// The box of a glyph in pixels: left, bottom, right and top, the baseline at 0.
struct glyph_box {
  int x0, y0, x1, y1;
};

consteval glyph_box box_of(const ttf::face& f, std::uint32_t g, float scale) {
  const std::size_t at = ttf::glyph_at(f, g);
  return {floor_of(static_cast<float>(ttf::i16(f.d, at + 2)) * scale), floor_of(static_cast<float>(ttf::i16(f.d, at + 4)) * scale),
          ceil_of(static_cast<float>(ttf::i16(f.d, at + 6)) * scale), ceil_of(static_cast<float>(ttf::i16(f.d, at + 8)) * scale)};
}

consteval float scale_of(const ttf::face& f, std::int32_t px) { return static_cast<float>(px) / static_cast<float>(f.units); }

// Draws glyph `g`: 4 bits a pixel, rows without a gap, the high half of a byte first.
// The buffers get their size once and are written through pointers: a push_back for
// each pixel costs the interpreter ten times as much.
consteval std::span<const std::uint8_t> bitmap_of(const ttf::face& f, std::uint32_t g, float scale, glyph_box box) {
  ttf::contours parts;
  ttf::outline(f, g, ttf::matrix{scale, 0, 0, -scale, static_cast<float>(-box.x0), static_cast<float>(box.y1)}, parts);
  coverage drawn{box.x1 - box.x0, box.y1 - box.y0};
  for (const auto& part : parts) ttf::draw(drawn, part);
  const auto count = static_cast<std::size_t>((box.x1 - box.x0) * (box.y1 - box.y0)), bytes = (count + 1) / 2;
  const auto pixels = std::make_unique_for_overwrite<unsigned char[]>(count);
  drawn.pixels(15, pixels.get());
  const auto out = std::make_unique_for_overwrite<std::uint8_t[]>(bytes);
  for (std::size_t i = 0; i < count; i += 2)
    out[i / 2] = static_cast<std::uint8_t>(pixels[i] << 4 | (i + 1 < count ? pixels[i + 1] : 0));
  return std::define_static_array(std::span<const std::uint8_t>(out.get(), bytes));
}

consteval void check_fits(glyph_box box, int advance) {
  if (box.x1 - box.x0 > 255 || box.y1 - box.y0 > 255 || advance > 4095 || box.x0 < -128 || box.y0 < -128)
    fail("lv: a glyph of this font is larger than LVGL's font format holds at this size (a box of 255 px). Use a smaller size.");
}

// How far the pen moves after glyph `g`, in sixteenths of a pixel.
consteval int advance_of(const ttf::face& f, std::uint32_t g, float scale) {
  return round_of(static_cast<float>(ttf::advance_of(f, g)) * scale * 16);
}

// A font file as a template argument: where its bytes are, and how many.
struct font_source {
  const unsigned char* data;
  std::size_t size;
};

// A glyph drawn at one size: its pixels, and its description. The description leaves the
// place of the pixels in the font's bitmap at 0.
struct drawn_glyph {
  std::span<const std::uint8_t> pixels;
  lv_font_fmt_txt_glyph_dsc_t dsc;
};

consteval drawn_glyph draw_glyph(font_source file, std::int32_t px, std::uint32_t g) {
  const ttf::face f = ttf::open({file.data, file.size});
  const float scale = scale_of(f, px);
  const int advance = advance_of(f, g, scale);
  lv_font_fmt_txt_glyph_dsc_t dsc{};
  dsc.adv_w = static_cast<std::uint32_t>(advance) & 0xfff;
  if (ttf::is_blank(f, g)) return {{}, dsc};
  const auto box = box_of(f, g, scale);
  check_fits(box, advance);
  dsc.box_w = static_cast<std::uint8_t>(box.x1 - box.x0), dsc.box_h = static_cast<std::uint8_t>(box.y1 - box.y0);
  dsc.ofs_x = static_cast<std::int8_t>(box.x0), dsc.ofs_y = static_cast<std::int8_t>(box.y0);
  return {bitmap_of(f, g, scale, box), dsc};
}

// Glyph `Glyph` of `File` at `Px` pixels. Each is a constant evaluation of its own, and
// GCC's -fconstexpr-ops-limit counts one evaluation, so the limit applies to one glyph.
template <font_source File, std::int32_t Px, std::uint32_t Glyph>
inline constexpr drawn_glyph glyph = draw_glyph(File, Px, Glyph);

consteval const drawn_glyph& drawn(font_source file, std::int32_t px, std::uint32_t g) {
  using std::meta::reflect_constant;
  return std::meta::extract<const drawn_glyph&>(
      std::meta::substitute(^^glyph, {reflect_constant(file), reflect_constant(px), reflect_constant(g)}));
}

consteval std::uint32_t glyph_for(const ttf::face& f, std::uint32_t character) {
  const auto g = ttf::glyph_of(f, character);
  if (g == 0) fail("lv: the font file has no glyph for the character U+" + hex(character) + ". Take it out of the font's characters.");
  return g;
}

// The glyphs of a font, in the order of its characters, as a template argument.
struct glyph_ids {
  const std::uint32_t* data;
  std::size_t size;
};

// The most bytes of pixels a font holds: LVGL's glyph description has 20 bits for the place.
inline constexpr std::size_t most_bitmap_bytes = 0xfffff;

// The description of each glyph of a font, glyph 0 ("none") first, and the bytes of all
// their pixels.
struct glyph_table {
  std::vector<lv_font_fmt_txt_glyph_dsc_t> glyphs{lv_font_fmt_txt_glyph_dsc_t{}};
  std::size_t bitmap_size = 0;
};

consteval glyph_table glyphs_of(font_source file, std::int32_t px, glyph_ids ids) {
  glyph_table table;
  for (std::size_t i = 0; i < ids.size; ++i) {
    const auto& g = drawn(file, px, ids.data[i]);
    auto dsc = g.dsc;
    dsc.bitmap_index = static_cast<std::uint32_t>(table.bitmap_size & most_bitmap_bytes);
    table.bitmap_size += g.pixels.size();
    table.glyphs.push_back(dsc);
  }
  return table;
}

// Checks the glyphs of a font against LVGL's format before any is drawn: the box of each,
// and the bytes of pixels of all, which the boxes give.
consteval void check_glyphs(const ttf::face& f, std::int32_t px, glyph_ids ids) {
  const float scale = scale_of(f, px);
  std::size_t bytes = 0;
  for (std::size_t i = 0; i < ids.size; ++i) {
    if (ttf::is_blank(f, ids.data[i])) continue;
    const auto box = box_of(f, ids.data[i], scale);
    check_fits(box, advance_of(f, ids.data[i], scale));
    bytes += (static_cast<std::size_t>((box.x1 - box.x0) * (box.y1 - box.y0)) + 1) / 2;
  }
  if (bytes > most_bitmap_bytes)
    fail("lv: the pixels of this font take " + decimal(bytes) + " bytes; LVGL's font format holds at most " +
         decimal(most_bitmap_bytes) + ". Use a smaller size or fewer characters.");
}

// The pixels of every glyph of a font, one after another. A `std::array` filled through
// pointers costs the interpreter a sixth of what std::define_static_array does for each
// byte, and a span's checked operator[] five times what a pointer does.
template <std::size_t Size>
consteval std::array<std::uint8_t, Size> joined(font_source file, std::int32_t px, glyph_ids ids) {
  std::array<std::uint8_t, Size> out{};
  std::uint8_t* to = out.data();
  for (std::size_t i = 0; i < ids.size; ++i) {
    const auto pixels = drawn(file, px, ids.data[i]).pixels;
    const std::uint8_t* from = pixels.data();
    const std::size_t count = pixels.size();
    for (std::size_t k = 0; k < count; ++k) to[k] = from[k];
    to += count;
  }
  return out;
}

// The bitmap of a font, in a constant evaluation of its own, as for `glyph`.
template <font_source File, std::int32_t Px, glyph_ids Ids, std::size_t Size>
inline constexpr std::array<std::uint8_t, Size> bitmap = joined<Size>(File, Px, Ids);

template <font_source File, std::int32_t Px, glyph_ids Ids, std::size_t Size>
inline constexpr const std::uint8_t* bitmap_data = bitmap<File, Px, Ids, Size>.data();

consteval const std::uint8_t* stored_bitmap(font_source file, std::int32_t px, glyph_ids ids, std::size_t size) {
  using std::meta::reflect_constant;
  return std::meta::extract<const std::uint8_t*>(std::meta::substitute(
      ^^bitmap_data, {reflect_constant(file), reflect_constant(px), reflect_constant(ids), reflect_constant(size)}));
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

// The glyphs of a font and the kerning tables read for them, as a template argument.
struct glyph_list {
  glyph_ids ids;
  const ttf::pair_table* tables;
  std::size_t table_count;
};

// A pair that the file moves: the place of its second glyph in the font, from 1, and
// how far, in sixteenths of a pixel.
struct kerned {
  std::uint8_t right;
  std::int8_t value;
};

consteval std::span<const kerned> kerning_of(font_source file, std::int32_t px, glyph_list list, std::size_t left) {
  const ttf::face f = ttf::open({file.data, file.size});
  const std::vector<std::uint32_t> glyphs(list.ids.data, list.ids.data + list.ids.size);
  const auto row = ttf::kerning_row(f, {list.tables, list.table_count}, glyphs[left], glyphs);
  std::vector<kerned> out;
  for (std::size_t r = 0; r < glyphs.size(); ++r)
    if (const auto v = kern_value(row[r], scale_of(f, px)); v != 0) out.push_back({static_cast<std::uint8_t>(r + 1), v});
  return std::define_static_array(out);
}

// The pairs that glyph `Left` of `Glyphs` begins, in a constant evaluation of its own, as
// for `glyph`: the kerning of a font of 95 characters is 9025 pairs.
template <font_source File, std::int32_t Px, glyph_list Glyphs, std::size_t Left>
inline constexpr std::span<const kerned> kerning_row = kerning_of(File, Px, Glyphs, Left);

consteval std::span<const kerned> kerned_row(font_source file, std::int32_t px, glyph_list glyphs, std::size_t left) {
  using std::meta::reflect_constant;
  return std::meta::extract<const std::span<const kerned>&>(std::meta::substitute(
      ^^kerning_row, {reflect_constant(file), reflect_constant(px), reflect_constant(glyphs), reflect_constant(left)}));
}

// The pairs of the font's glyphs that the file moves, in the order LVGL searches them.
consteval const lv_font_fmt_txt_kern_pair_t* stored_kerning(const ttf::face& f, font_source file, std::int32_t px,
                                                            glyph_ids ids) {
  const auto tables = std::define_static_array(ttf::pair_tables(f, {ids.data, ids.data + ids.size}));
  if (tables.empty()) return nullptr;
  std::vector<std::uint8_t> pair_ids;
  std::vector<std::int8_t> values;
  for (std::size_t l = 0; l < ids.size; ++l)
    for (const auto pair : kerned_row(file, px, {ids, tables.data(), tables.size()}, l)) {
      pair_ids.push_back(static_cast<std::uint8_t>(l + 1)), pair_ids.push_back(pair.right);
      values.push_back(pair.value);
    }
  if (values.empty()) return nullptr;
  lv_font_fmt_txt_kern_pair_t pairs{};
  pairs.glyph_ids = std::define_static_array(pair_ids).data();
  pairs.values = std::define_static_array(values).data();
  pairs.pair_cnt = static_cast<std::uint32_t>(values.size()) & 0x3fffffff;
  pairs.glyph_ids_size = 0;  // one byte for each glyph
  return std::define_static_array(std::vector{pairs}).data();
}

consteval const lv_font_fmt_txt_dsc_t* stored_dsc(const ttf::face& f, font_source file, std::int32_t px,
                                                  const std::vector<std::uint32_t>& characters) {
  std::vector<std::uint32_t> glyphs;
  for (const auto c : characters) glyphs.push_back(glyph_for(f, c));
  const auto list = std::define_static_array(glyphs);
  const glyph_ids ids{list.data(), list.size()};
  check_glyphs(f, px, ids);
  const auto table = glyphs_of(file, px, ids);
  lv_font_fmt_txt_dsc_t dsc{};
  dsc.glyph_bitmap = stored_bitmap(file, px, ids, table.bitmap_size);
  dsc.glyph_dsc = std::define_static_array(table.glyphs).data();
  dsc.cmaps = stored_cmap(characters);
  dsc.kern_dsc = stored_kerning(f, file, px, ids);
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
  const float scale = scale_of(f, px);
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
  font.dsc = stored_dsc(f, {file.data(), file.size()}, px, characters);
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

// The characters from the space to the tilde and `Extra` on top of them, as the text for the characters of a
// font: lv::ascii_plus<"€·"> for a price and a separator.
template <detail::fixed Extra>
inline constexpr std::string_view ascii_plus = std::define_static_string(detail::ascii_plus_text(Extra.sv()));

// The default characters and the accents of Latin-1 (U+00A0 to U+00FF), 191 characters, and `Extra` on top:
// lv::latin1_plus<"€"> for a price in a language with accents. It is the dear one: 96 more glyphs than the default
// set, about 86 G instructions of compile time (a build of 11 seconds and 0.9 GB) and 12 KB of flash for each font at 16 px (docs/fonts.md). Name the
// accents that a text needs with lv::ascii_plus when it needs a few.
template <detail::fixed Extra>
inline constexpr std::string_view latin1_plus = std::define_static_string(detail::ascii_plus_text(Extra.sv(), "lv::latin1_plus", true));

// The default characters and the accents of Latin-1: the same cost as lv::latin1_plus<"">.
inline constexpr std::string_view latin1 = latin1_plus<"">;

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
  // A design without fonts: the project's looks hold no typography token.
  consteval typeface() = default;

  consteval typeface(std::initializer_list<face_file> files) {
    if (files.size() > files_.size()) detail::fail("lv: a typeface has at most eight files.");
    for (const auto& f : files) files_[count_++] = f;
  }

  // The number of the file that a typography token asks for.
  consteval std::size_t file_of(const token<typography_token>& t) const {
    for (std::size_t i = 0; i < count_; ++i)
      if (files_[i].family == t->font_family && files_[i].weight == t->font_weight) return i;
    detail::fail("lv: the typeface has no file for the family '" + detail::str(t->font_family) + "' at weight " +
                 detail::decimal(static_cast<std::size_t>(t->font_weight)) + ". Add it: lv::typeface({{\"" + detail::str(t->font_family) + "\", " +
                 detail::decimal(static_cast<std::size_t>(t->font_weight)) + ", its_ttf}}).");
  }

  // The font of file number `file` at `px` pixels.
  consteval const lv_font_t* draw(std::size_t file, std::int32_t px) const {
    return detail::make_font(files_[file].file, px, files_[file].characters);
  }

  /// The font of a typography token: its family, weight and size.
  consteval token<const lv_font_t*> operator()(const token<typography_token>& t) const {
    return {draw(file_of(t), t->font_size.raw()), detail::from_design_tokens{}};
  }

 private:
  std::array<face_file, 8> files_{};
  std::size_t count_ = 0;
};

}  // namespace lv
