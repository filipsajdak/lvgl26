#pragma once

// A TrueType file, read in constant evaluation: which glyph a character has, its
// outline and advance, and the kerning of a pair.
//
// Reads head, maxp, hhea, hmtx, cmap (format 4), loca, glyf (simple and composite
// glyphs) and GPOS (pair adjustment, formats 1 and 2). It applies no hinting.

#include <lv/detail/api.hpp>
#include <lv/detail/coverage.hpp>

#include <cstddef>
#include <cstdint>
#include <meta>
#include <span>
#include <string_view>
#include <vector>

namespace lv::detail::ttf {

using bytes = std::span<const unsigned char>;

consteval std::uint32_t u16(bytes d, std::size_t at) { return static_cast<std::uint32_t>(d[at] << 8 | d[at + 1]); }
consteval std::int32_t i16(bytes d, std::size_t at) { return static_cast<std::int16_t>(u16(d, at)); }
consteval std::uint32_t u32(bytes d, std::size_t at) { return u16(d, at) << 16 | u16(d, at + 2); }

struct face {
  bytes d;
  std::size_t head = 0, maxp = 0, hhea = 0, hmtx = 0, cmap = 0, loca = 0, glyf = 0, gpos = 0;
  std::size_t cmap4 = 0;  // the format 4 subtable of cmap
  std::uint32_t units = 0, glyphs = 0, h_metrics = 0;
  bool long_loca = false;
};

consteval bool is_tag(bytes d, std::size_t at, std::string_view tag) {
  for (std::size_t i = 0; i < 4; ++i)
    if (d[at + i] != static_cast<unsigned char>(tag[i])) return false;
  return true;
}

// Where the table called `tag` starts, or 0.
consteval std::size_t table(bytes d, std::string_view tag) {
  for (std::size_t i = 0; i < u16(d, 4); ++i)
    if (is_tag(d, 12 + 16 * i, tag)) return u32(d, 12 + 16 * i + 8);
  return 0;
}

consteval std::size_t format4_of(bytes d, std::size_t cmap) {
  for (std::size_t i = 0; i < u16(d, cmap + 2); ++i) {
    const std::size_t at = cmap + u32(d, cmap + 4 + 8 * i + 4);
    if (u16(d, at) == 4) return at;
  }
  return 0;
}

consteval void check(const face& f) {
  if (f.glyf == 0 || f.loca == 0)
    fail("lv: this font has no TrueType outlines (a .otf file with CFF outlines has none). Use the .ttf form of the font.");
  if (f.head == 0 || f.maxp == 0 || f.hhea == 0 || f.hmtx == 0 || f.cmap4 == 0)
    fail("lv: this font file lacks a table that lvgl26 reads (head, maxp, hhea, hmtx, or a format 4 cmap).");
}

consteval face open(bytes d) {
  if (d.size() < 12 || (u32(d, 0) != 0x00010000 && !is_tag(d, 0, "true") && !is_tag(d, 0, "OTTO")))
    fail("lv: this is not a TrueType font file. Embed a .ttf file: `constexpr unsigned char font[] = { #embed \"Name.ttf\" };`");
  face f{d};
  f.head = table(d, "head"), f.maxp = table(d, "maxp"), f.hhea = table(d, "hhea"), f.hmtx = table(d, "hmtx");
  f.cmap = table(d, "cmap"), f.loca = table(d, "loca"), f.glyf = table(d, "glyf"), f.gpos = table(d, "GPOS");
  f.cmap4 = f.cmap == 0 ? 0 : format4_of(d, f.cmap);
  check(f);
  f.units = u16(d, f.head + 18);
  f.long_loca = i16(d, f.head + 50) != 0;
  f.glyphs = u16(d, f.maxp + 4);
  f.h_metrics = u16(d, f.hhea + 34);
  return f;
}

// The glyph of a character, or 0 when the font has none.
consteval std::uint32_t glyph_of(const face& f, std::uint32_t c) {
  const bytes d = f.d;
  const std::size_t segments = u16(d, f.cmap4 + 6) / 2;
  const std::size_t ends = f.cmap4 + 14, starts = ends + 2 * segments + 2;
  const std::size_t deltas = starts + 2 * segments, offsets = deltas + 2 * segments;
  std::size_t s = 0;
  while (s < segments && u16(d, ends + 2 * s) < c) ++s;
  if (c > 0xffff || s == segments || u16(d, starts + 2 * s) > c) return 0;
  const auto offset = u16(d, offsets + 2 * s), delta = u16(d, deltas + 2 * s);
  if (offset == 0) return (c + delta) & 0xffff;
  const auto g = u16(d, offsets + 2 * s + offset + 2 * (c - u16(d, starts + 2 * s)));
  return g == 0 ? 0 : (g + delta) & 0xffff;
}

consteval std::size_t glyph_at(const face& f, std::uint32_t g) {
  return f.glyf + (f.long_loca ? u32(f.d, f.loca + 4 * g) : 2 * u16(f.d, f.loca + 2 * g));
}
// A glyph without an outline, such as a space.
consteval bool is_blank(const face& f, std::uint32_t g) { return glyph_at(f, g) == glyph_at(f, g + 1); }

// How far the pen moves after the glyph, in font units.
consteval std::uint32_t advance_of(const face& f, std::uint32_t g) {
  return u16(f.d, f.hmtx + 4 * (g < f.h_metrics ? g : f.h_metrics - 1));
}

// x' = a x + c y + e, y' = b x + d y + f
struct matrix {
  float a = 1, b = 0, c = 0, d = 1, e = 0, f = 0;
};

consteval matrix operator*(const matrix& m, const matrix& n) {
  return {m.a * n.a + m.c * n.b, m.b * n.a + m.d * n.b, m.a * n.c + m.c * n.d,
          m.b * n.c + m.d * n.d, m.a * n.e + m.c * n.f + m.e, m.b * n.e + m.d * n.f + m.f};
}

using contours = std::vector<std::vector<outline_point>>;

// The flags of the points of a simple glyph. `p` moves past them.
consteval std::vector<unsigned char> point_flags(bytes d, std::size_t& p, std::size_t points) {
  std::vector<unsigned char> flags;
  while (flags.size() < points) {
    const unsigned char flag = d[p++];
    flags.push_back(flag);
    if (flag & 8)
      for (unsigned repeat = d[p++]; repeat > 0; --repeat) flags.push_back(flag);
  }
  return flags;
}

// One coordinate of every point. A flag says how each is stored: `is_byte` a byte
// with its sign in `same`, otherwise two bytes, or nothing when `same` (no change).
consteval std::vector<int> coordinates(bytes d, std::size_t& p, const std::vector<unsigned char>& flags,
                                       unsigned is_byte, unsigned same) {
  std::vector<int> out;
  int v = 0;
  for (const unsigned char flag : flags) {
    if (flag & is_byte) {
      v += (flag & same) ? d[p] : -static_cast<int>(d[p]);
      p += 1;
    } else if (!(flag & same)) {
      v += i16(d, p);
      p += 2;
    }
    out.push_back(v);
  }
  return out;
}

consteval void simple_outline(const face& f, std::size_t at, std::size_t count, const matrix& m, contours& out) {
  const bytes d = f.d;
  const std::size_t ends = at + 10, points = u16(d, ends + 2 * (count - 1)) + 1;
  std::size_t p = ends + 2 * count;
  p += 2 + u16(d, p);  // the instructions
  const auto flags = point_flags(d, p, points);
  const auto xs = coordinates(d, p, flags, 2, 16), ys = coordinates(d, p, flags, 4, 32);
  std::size_t i = 0;
  for (std::size_t c = 0; c < count; ++c) {
    std::vector<outline_point> contour;
    for (; i <= u16(d, ends + 2 * c); ++i) {
      const auto x = static_cast<float>(xs[i]), y = static_cast<float>(ys[i]);
      contour.push_back({m.a * x + m.c * y + m.e, m.b * x + m.d * y + m.f, (flags[i] & 1) != 0});
    }
    out.push_back(contour);
  }
}

consteval float f2dot14(bytes d, std::size_t at) { return static_cast<float>(i16(d, at)) / 16384.0f; }

// The placement of one part of a composite glyph. `p` moves past it.
consteval matrix part_matrix(bytes d, std::size_t& p, std::uint32_t flags) {
  matrix m;
  if (flags & 1) {
    m.e = static_cast<float>(i16(d, p)), m.f = static_cast<float>(i16(d, p + 2));
    p += 4;
  } else {
    m.e = static_cast<float>(static_cast<signed char>(d[p])), m.f = static_cast<float>(static_cast<signed char>(d[p + 1]));
    p += 2;
  }
  if (flags & 8) {
    m.a = m.d = f2dot14(d, p);
    p += 2;
  } else if (flags & 0x40) {
    m.a = f2dot14(d, p), m.d = f2dot14(d, p + 2);
    p += 4;
  } else if (flags & 0x80) {
    m.a = f2dot14(d, p), m.b = f2dot14(d, p + 2), m.c = f2dot14(d, p + 4), m.d = f2dot14(d, p + 6);
    p += 8;
  }
  return m;
}

// The outline of a glyph, placed by `m`. A composite glyph is made of other glyphs.
consteval void outline(const face& f, std::uint32_t g, const matrix& m, contours& out, int depth = 0) {
  if (is_blank(f, g) || depth > 4) return;
  const std::size_t at = glyph_at(f, g);
  if (i16(f.d, at) >= 0) return simple_outline(f, at, u16(f.d, at), m, out);
  std::size_t p = at + 10;
  for (bool more = true; more;) {
    const auto flags = u16(f.d, p), part = u16(f.d, p + 2);
    p += 4;
    outline(f, part, m * part_matrix(f.d, p, flags), out, depth + 1);
    more = (flags & 0x20) != 0;
  }
}

consteval outline_point between(outline_point a, outline_point b) { return {(a.x + b.x) / 2, (a.y + b.y) / 2, true}; }

// The points of a closed contour, beginning with one on the curve. When the file has
// none on the curve, the point half way between its first two control points begins.
consteval std::vector<outline_point> from_the_curve(const std::vector<outline_point>& c) {
  std::size_t first = 0;
  while (first < c.size() && !c[first].on) ++first;
  std::vector<outline_point> out;
  if (first == c.size()) out.push_back(between(c[0], c[1])), first = 0;
  const std::size_t skip = out.size();  // the first control point comes round again at the end
  for (std::size_t k = skip; k < c.size() + skip; ++k) out.push_back(c[(first + k) % c.size()]);
  return out;
}

// Two control points in a row have a point on the curve half way between them, which
// the file leaves out. This writes those points out.
consteval std::vector<outline_point> with_implied_points(const std::vector<outline_point>& c) {
  std::vector<outline_point> out;
  for (const auto p : c) {
    if (!p.on && !out.empty() && !out.back().on) out.push_back(between(out.back(), p));
    out.push_back(p);
  }
  return out;
}

// Draws a closed contour: lines between points on the curve, a curve around each
// control point.
consteval void draw(coverage& to, const std::vector<outline_point>& contour) {
  if (contour.size() < 2) return;
  const auto c = with_implied_points(from_the_curve(contour));
  outline_point at = c[0];
  for (std::size_t i = 1; i < c.size(); ++i) {
    if (c[i].on) {
      to.line(at, c[i]);
      at = c[i];
    } else {
      const outline_point end = i + 1 < c.size() ? c[i + 1] : c[0];
      to.curve(at, c[i], end);
      at = end;
      ++i;
    }
  }
  to.line(at, c[0]);
}

// ---- kerning: GPOS, lookup type 2 -----------------------------------------------

// The place of glyph `g` in a coverage table, or -1.
consteval int covered(bytes d, std::size_t t, std::uint32_t g) {
  const std::size_t count = u16(d, t + 2);
  for (std::size_t i = 0; i < count; ++i) {
    if (u16(d, t) == 1 && u16(d, t + 4 + 2 * i) == g) return static_cast<int>(i);
    const std::size_t r = t + 4 + 6 * i;
    if (u16(d, t) == 2 && u16(d, r) <= g && g <= u16(d, r + 2)) return static_cast<int>(u16(d, r + 4) + g - u16(d, r));
  }
  return -1;
}

// The class of glyph `g` in a class definition table. A glyph that is not named has class 0.
consteval std::uint32_t class_of(bytes d, std::size_t t, std::uint32_t g) {
  if (u16(d, t) == 1) {
    const auto first = u16(d, t + 2);
    return g >= first && g < first + u16(d, t + 4) ? u16(d, t + 6 + 2 * (g - first)) : 0;
  }
  for (std::size_t i = 0; i < u16(d, t + 2); ++i) {
    const std::size_t r = t + 4 + 6 * i;
    if (u16(d, r) <= g && g <= u16(d, r + 2)) return u16(d, r + 4);
  }
  return 0;
}

consteval std::size_t bits(std::uint32_t v) {
  std::size_t n = 0;
  for (; v != 0; v >>= 1) n += v & 1;
  return n;
}

// The change of the first glyph's advance in a value record, in font units.
consteval int x_advance(bytes d, std::size_t record, std::uint32_t format) {
  return (format & 4) ? i16(d, record + 2 * bits(format & 3)) : 0;
}

// What a subtable says about a pair: whether it has a say at all, and how many font
// units the first glyph's advance changes.
struct pair_answer {
  bool applies;
  int units;
};

// Pairs listed one by one for each first glyph. A pair that is not listed is left
// to the next subtable.
consteval pair_answer pair_format1(bytes d, std::size_t t, int first, std::uint32_t right) {
  const auto format1 = u16(d, t + 4), format2 = u16(d, t + 6);
  const std::size_t set = t + u16(d, t + 10 + 2 * static_cast<std::size_t>(first));
  const std::size_t size = 2 + 2 * (bits(format1) + bits(format2));
  for (std::size_t i = 0; i < u16(d, set); ++i)
    if (u16(d, set + 2 + size * i) == right) return {true, x_advance(d, set + 2 + size * i + 2, format1)};
  return {false, 0};
}

// Pairs given by the class of each glyph: every pair has an entry. `first_class` and
// `second_class` are the classes of the two glyphs in this subtable.
consteval pair_answer pair_format2(bytes d, std::size_t t, std::uint32_t first_class, std::uint32_t second_class) {
  const auto format1 = u16(d, t + 4), format2 = u16(d, t + 6);
  const std::size_t size = 2 * (bits(format1) + bits(format2));
  return {true, x_advance(d, t + 16 + size * (first_class * u16(d, t + 14) + second_class), format1)};
}

// The pair adjustment subtables of one lookup. A lookup of type 9 wraps another one.
consteval void add_pair_subtables(bytes d, std::size_t lookup, std::vector<std::size_t>& out) {
  for (std::size_t k = 0; k < u16(d, lookup + 4); ++k) {
    std::size_t t = lookup + u16(d, lookup + 6 + 2 * k);
    auto type = u16(d, lookup);
    if (type == 9) type = u16(d, t + 2), t += u32(d, t + 4);
    if (type == 2) out.push_back(t);
  }
}

// The pair adjustment subtables of the font's kerning: the lookups of its first
// `kern` feature. A font may kern differently for another script; that is not read.
consteval std::vector<std::size_t> pair_subtables(const face& f) {
  std::vector<std::size_t> out;
  if (f.gpos == 0) return out;
  const bytes d = f.d;
  const std::size_t features = f.gpos + u16(d, f.gpos + 6), lookups = f.gpos + u16(d, f.gpos + 8);
  for (std::size_t i = 0; i < u16(d, features); ++i) {
    if (!is_tag(d, features + 2 + 6 * i, "kern")) continue;
    const std::size_t feature = features + u16(d, features + 2 + 6 * i + 4);
    for (std::size_t k = 0; k < u16(d, feature + 2); ++k)
      add_pair_subtables(d, lookups + u16(d, lookups + 2 + 2 * u16(d, feature + 4 + 2 * k)), out);
    return out;
  }
  return out;
}

// A pair adjustment subtable, and what it says about each glyph of a font as the second
// glyph of a pair. A font of 95 characters has 9025 pairs. Whatever is true of one
// glyph is therefore read once for the font, and not once for every pair.
struct pair_table {
  std::size_t at;
  const std::uint32_t* second_class;  // format 2: the class of each glyph of the font, in static storage
};

// The class of each of `glyphs` as the second glyph of a pair. Only format 2 has classes.
consteval std::vector<std::uint32_t> second_classes(bytes d, std::size_t t, const std::vector<std::uint32_t>& glyphs) {
  std::vector<std::uint32_t> out;
  if (u16(d, t) != 2) return out;
  for (const auto g : glyphs) out.push_back(class_of(d, t + u16(d, t + 10), g));
  return out;
}

// The kerning subtables of the font, read for the glyphs of one font made from it.
consteval std::vector<pair_table> pair_tables(const face& f, const std::vector<std::uint32_t>& glyphs) {
  std::vector<pair_table> out;
  for (const std::size_t t : pair_subtables(f)) out.push_back({t, std::define_static_array(second_classes(f.d, t, glyphs)).data()});
  return out;
}

// A subtable that lists its pairs: what it says about `first`, a place in its coverage,
// with each of `glyphs`. An answer that is there already stays.
consteval void add_listed(bytes d, std::size_t t, int first, const std::vector<std::uint32_t>& glyphs,
                          std::vector<pair_answer>& row) {
  for (std::size_t r = 0; r < row.size(); ++r)
    if (!row[r].applies) row[r] = pair_format1(d, t, first, glyphs[r]);
}

// A subtable with classes: what it says about a first glyph of `first_class` with each
// glyph of the font. An answer that is there already stays.
consteval void add_classed(bytes d, const pair_table& table, std::uint32_t first_class, std::vector<pair_answer>& row) {
  for (std::size_t r = 0; r < row.size(); ++r)
    if (!row[r].applies) row[r] = pair_format2(d, table.at, first_class, table.second_class[r]);
}

// Adds what one subtable says about the first glyph `left` with each of `glyphs`. The
// first subtable that has a say decides, so the subtables are asked in their order.
consteval void add_answers(bytes d, const pair_table& table, std::uint32_t left, const std::vector<std::uint32_t>& glyphs,
                           std::vector<pair_answer>& row) {
  const std::size_t t = table.at;
  const int first = covered(d, t + u16(d, t + 2), left);
  if (first < 0) return;
  if (u16(d, t) == 1) add_listed(d, t, first, glyphs, row);
  if (u16(d, t) == 2) add_classed(d, table, class_of(d, t + u16(d, t + 8), left), row);
}

// How much closer or further apart `left` and each of `glyphs` stand than the advance
// of `left` says, in font units.
consteval std::vector<int> kerning_row(const face& f, std::span<const pair_table> tables, std::uint32_t left,
                                       const std::vector<std::uint32_t>& glyphs) {
  std::vector<pair_answer> row(glyphs.size(), pair_answer{false, 0});
  for (const auto& table : tables) add_answers(f.d, table, left, glyphs, row);
  std::vector<int> units;
  for (const auto& answer : row) units.push_back(answer.units);
  return units;
}

}  // namespace lv::detail::ttf
