#pragma once

// Spike: a TrueType file becomes a constant lv_font_t, in constant evaluation.
// No converter, no heap, no rasteriser on the device.
//
// Reads: head, maxp, hhea, hmtx, cmap (format 4), loca, glyf (simple and composite).
// Does not read: kerning (GPOS), hinting, CFF outlines.
//
// The coverage method in `raster::line` follows font-rs (https://github.com/raphlinus/font-rs),
// which is under the Apache License 2.0. Code that grows out of this spike keeps that notice.

#include <lvgl/lvgl.h>

#include <cstddef>
#include <cstdint>
#include <meta>
#include <span>
#include <vector>

namespace spike {

using bytes = std::span<const unsigned char>;

consteval std::uint32_t u16(bytes d, std::size_t at) { return static_cast<std::uint32_t>(d[at] << 8 | d[at + 1]); }
consteval std::int32_t i16(bytes d, std::size_t at) { return static_cast<std::int16_t>(u16(d, at)); }
consteval std::uint32_t u32(bytes d, std::size_t at) { return u16(d, at) << 16 | u16(d, at + 2); }

struct face {
  bytes d;
  std::size_t head = 0, maxp = 0, hhea = 0, hmtx = 0, cmap = 0, loca = 0, glyf = 0;
  std::uint32_t units = 0, glyphs = 0, h_metrics = 0;
  bool long_loca = false;
  std::size_t cmap4 = 0;  // the format 4 subtable
};

consteval bool is_tag(bytes d, std::size_t at, const char* tag) {
  for (std::size_t i = 0; i < 4; ++i)
    if (d[at + i] != static_cast<unsigned char>(tag[i])) return false;
  return true;
}

consteval face open(bytes d) {
  face f{d};
  const auto count = u16(d, 4);
  for (std::size_t i = 0; i < count; ++i) {
    const std::size_t rec = 12 + 16 * i, at = u32(d, rec + 8);
    if (is_tag(d, rec, "head")) f.head = at;
    if (is_tag(d, rec, "maxp")) f.maxp = at;
    if (is_tag(d, rec, "hhea")) f.hhea = at;
    if (is_tag(d, rec, "hmtx")) f.hmtx = at;
    if (is_tag(d, rec, "cmap")) f.cmap = at;
    if (is_tag(d, rec, "loca")) f.loca = at;
    if (is_tag(d, rec, "glyf")) f.glyf = at;
  }
  f.units = u16(d, f.head + 18);
  f.long_loca = i16(d, f.head + 50) != 0;
  f.glyphs = u16(d, f.maxp + 4);
  f.h_metrics = u16(d, f.hhea + 34);
  const auto subtables = u16(d, f.cmap + 2);
  for (std::size_t i = 0; i < subtables; ++i) {
    const std::size_t at = f.cmap + u32(d, f.cmap + 4 + 8 * i + 4);
    if (u16(d, at) == 4) f.cmap4 = at;
  }
  return f;
}

// The glyph of a character, from the format 4 table.
consteval std::uint32_t glyph_of(const face& f, std::uint32_t c) {
  const bytes d = f.d;
  const std::size_t t = f.cmap4, segs = u16(d, t + 6) / 2;
  const std::size_t ends = t + 14, starts = ends + 2 * segs + 2, deltas = starts + 2 * segs, offsets = deltas + 2 * segs;
  for (std::size_t s = 0; s < segs; ++s) {
    if (u16(d, ends + 2 * s) < c) continue;
    const auto start = u16(d, starts + 2 * s);
    if (start > c) return 0;
    const auto offset = u16(d, offsets + 2 * s);
    if (offset == 0) return (c + u16(d, deltas + 2 * s)) & 0xffff;
    const auto g = u16(d, offsets + 2 * s + offset + 2 * (c - start));
    return g == 0 ? 0 : (g + u16(d, deltas + 2 * s)) & 0xffff;
  }
  return 0;
}

consteval std::size_t glyph_at(const face& f, std::uint32_t g) {
  return f.glyf + (f.long_loca ? u32(f.d, f.loca + 4 * g) : 2 * u16(f.d, f.loca + 2 * g));
}
consteval bool is_blank(const face& f, std::uint32_t g) { return glyph_at(f, g) == glyph_at(f, g + 1); }

consteval std::uint32_t advance_of(const face& f, std::uint32_t g) {
  return u16(f.d, f.hmtx + 4 * (g < f.h_metrics ? g : f.h_metrics - 1));
}

struct point {
  float x, y;
  bool on;
};

// x' = a x + c y + e, y' = b x + d y + f
struct matrix {
  float a = 1, b = 0, c = 0, d = 1, e = 0, f = 0;
};

using contours = std::vector<std::vector<point>>;

consteval void simple_outline(const face& f, std::size_t at, int count, const matrix& m, contours& out) {
  const bytes d = f.d;
  std::vector<std::uint32_t> ends;
  for (int i = 0; i < count; ++i) ends.push_back(u16(d, at + 10 + 2 * static_cast<std::size_t>(i)));
  const std::size_t points = ends.back() + 1;
  std::size_t p = at + 10 + 2 * static_cast<std::size_t>(count);
  p += 2 + u16(d, p);  // the instructions
  std::vector<unsigned char> flags;
  while (flags.size() < points) {
    const unsigned char flag = d[p++];
    flags.push_back(flag);
    if (flag & 8)
      for (unsigned repeat = d[p++]; repeat > 0; --repeat) flags.push_back(flag);
  }
  std::vector<int> xs(points), ys(points);
  int v = 0;
  for (std::size_t i = 0; i < points; ++i) {
    if (flags[i] & 2) {
      const int delta = d[p++];
      v += (flags[i] & 16) ? delta : -delta;
    } else if (!(flags[i] & 16)) {
      v += i16(d, p);
      p += 2;
    }
    xs[i] = v;
  }
  v = 0;
  for (std::size_t i = 0; i < points; ++i) {
    if (flags[i] & 4) {
      const int delta = d[p++];
      v += (flags[i] & 32) ? delta : -delta;
    } else if (!(flags[i] & 32)) {
      v += i16(d, p);
      p += 2;
    }
    ys[i] = v;
  }
  std::size_t first = 0;
  for (auto end : ends) {
    std::vector<point> contour;
    for (std::size_t i = first; i <= end; ++i) {
      const auto x = static_cast<float>(xs[i]), y = static_cast<float>(ys[i]);
      contour.push_back({m.a * x + m.c * y + m.e, m.b * x + m.d * y + m.f, (flags[i] & 1) != 0});
    }
    out.push_back(contour);
    first = end + 1;
  }
}

consteval float f2dot14(bytes d, std::size_t at) { return static_cast<float>(i16(d, at)) / 16384.0f; }

consteval void outline(const face& f, std::uint32_t g, const matrix& m, contours& out, int depth = 0) {
  if (is_blank(f, g) || depth > 4) return;
  const bytes d = f.d;
  const std::size_t at = glyph_at(f, g);
  const int count = i16(d, at);
  if (count >= 0) return simple_outline(f, at, count, m, out);
  std::size_t p = at + 10;
  for (;;) {
    const auto flags = u16(d, p), part = u16(d, p + 2);
    p += 4;
    matrix own;
    if (flags & 1) {
      own.e = static_cast<float>(i16(d, p));
      own.f = static_cast<float>(i16(d, p + 2));
      p += 4;
    } else {
      own.e = static_cast<float>(static_cast<signed char>(d[p]));
      own.f = static_cast<float>(static_cast<signed char>(d[p + 1]));
      p += 2;
    }
    if (flags & 8) {
      own.a = own.d = f2dot14(d, p);
      p += 2;
    } else if (flags & 0x40) {
      own.a = f2dot14(d, p);
      own.d = f2dot14(d, p + 2);
      p += 4;
    } else if (flags & 0x80) {
      own.a = f2dot14(d, p);
      own.b = f2dot14(d, p + 2);
      own.c = f2dot14(d, p + 4);
      own.d = f2dot14(d, p + 6);
      p += 8;
    }
    const matrix both{m.a * own.a + m.c * own.b, m.b * own.a + m.d * own.b, m.a * own.c + m.c * own.d,
                      m.b * own.c + m.d * own.d, m.a * own.e + m.c * own.f + m.e, m.b * own.e + m.d * own.f + m.f};
    outline(f, part, both, out, depth + 1);
    if (!(flags & 0x20)) return;
  }
}

consteval int floor_of(float v) {
  const int i = static_cast<int>(v);
  return static_cast<float>(i) > v ? i - 1 : i;
}
consteval int ceil_of(float v) { return -floor_of(-v); }

// Signed area coverage, accumulated along each row: the method of font-rs.
struct raster {
  int w, h;
  std::vector<float> a;

  consteval raster(int width, int height) : w(width), h(height), a(static_cast<std::size_t>((width + 2) * height + 2)) {}

  consteval float& cell(int x, int y) {
    x = x < 0 ? 0 : x > w ? w : x;
    return a[static_cast<std::size_t>(y * (w + 2) + x)];
  }

  consteval void line(point p0, point p1) {
    if (p0.y == p1.y) return;
    float dir = 1;
    if (p0.y > p1.y) {
      dir = -1;
      std::swap(p0, p1);
    }
    const float dxdy = (p1.x - p0.x) / (p1.y - p0.y);
    float x = p0.x;
    if (p0.y < 0) x -= p0.y * dxdy;
    const int last = ceil_of(p1.y) < h ? ceil_of(p1.y) : h;
    for (int y = p0.y < 0 ? 0 : floor_of(p0.y); y < last; ++y) {
      const float top = static_cast<float>(y) > p0.y ? static_cast<float>(y) : p0.y;
      const float bottom = static_cast<float>(y + 1) < p1.y ? static_cast<float>(y + 1) : p1.y;
      const float dy = bottom - top, xnext = x + dxdy * dy, d = dy * dir;
      const float x0 = x < xnext ? x : xnext, x1 = x < xnext ? xnext : x;
      const int x0i = floor_of(x0), x1i = ceil_of(x1);
      const float x0floor = static_cast<float>(x0i);
      if (x1i <= x0i + 1) {
        const float xmf = 0.5f * (x + xnext) - x0floor;
        cell(x0i, y) += d - d * xmf;
        cell(x0i + 1, y) += d * xmf;
      } else {
        const float s = 1.0f / (x1 - x0), x0f = x0 - x0floor;
        const float a0 = 0.5f * s * (1 - x0f) * (1 - x0f);
        const float x1f = x1 - static_cast<float>(x1i) + 1, am = 0.5f * s * x1f * x1f;
        cell(x0i, y) += d * a0;
        if (x1i == x0i + 2) {
          cell(x0i + 1, y) += d * (1 - a0 - am);
        } else {
          const float a1 = s * (1.5f - x0f);
          cell(x0i + 1, y) += d * (a1 - a0);
          for (int xi = x0i + 2; xi < x1i - 1; ++xi) cell(xi, y) += d * s;
          const float a2 = a1 + static_cast<float>(x1i - x0i - 3) * s;
          cell(x1i - 1, y) += d * (1 - a2 - am);
        }
        cell(x1i, y) += d * am;
      }
      x = xnext;
    }
  }

  consteval void quad(point p0, point p1, point p2) {
    const float devx = p0.x - 2 * p1.x + p2.x, devy = p0.y - 2 * p1.y + p2.y, devsq = devx * devx + devy * devy;
    int n = 1;
    while (static_cast<float>(n * n * n * n) < 3 * devsq) ++n;  // the pieces a curve of this bend needs
    point p = p0;
    for (int i = 1; i < n; ++i) {
      const float t = static_cast<float>(i) / static_cast<float>(n), u = 1 - t;
      const point next{u * u * p0.x + 2 * u * t * p1.x + t * t * p2.x, u * u * p0.y + 2 * u * t * p1.y + t * t * p2.y, true};
      line(p, next);
      p = next;
    }
    line(p, p2);
  }

  // A closed contour of on-curve and control points. Two control points in a row
  // have an on-curve point half way between them.
  consteval void contour(const std::vector<point>& c) {
    const std::size_t n = c.size();
    if (n < 2) return;
    const auto mid = [](point p, point q) { return point{(p.x + q.x) / 2, (p.y + q.y) / 2, true}; };
    std::size_t first = 0;
    while (first < n && !c[first].on) ++first;
    const point start = first < n ? c[first] : mid(c[0], c[1]);
    if (first == n) first = 0;
    point at = start;
    bool has_control = false;
    point control{};
    for (std::size_t k = 1; k <= n; ++k) {
      const point p = k == n && first < n && c[first].on ? start : c[(first + k) % n];
      if (k == n && !(first < n && c[first].on)) break;
      if (p.on) {
        has_control ? quad(at, control, p) : line(at, p);
        at = p;
        has_control = false;
      } else {
        if (has_control) {
          const point between = mid(control, p);
          quad(at, control, between);
          at = between;
        }
        control = p;
        has_control = true;
      }
    }
    if (has_control) quad(at, control, start);
    else if (at.x != start.x || at.y != start.y) line(at, start);
  }

  // The coverage of pixel (x, y), from 0 to 15.
  consteval std::vector<unsigned char> coverage() const {
    std::vector<unsigned char> out;
    for (int y = 0; y < h; ++y) {
      float acc = 0;
      for (int x = 0; x < w; ++x) {
        acc += a[static_cast<std::size_t>(y * (w + 2) + x)];
        const float v = acc < 0 ? -acc : acc;
        out.push_back(static_cast<unsigned char>((v > 1 ? 1 : v) * 15.0f + 0.5f));
      }
    }
    return out;
  }
};

struct options {
  int px;
  std::uint32_t first = 0x20, last = 0x7e;
};

consteval int round_of(float v) { return floor_of(v + 0.5f); }

// The font as a constant with static storage: bitmaps at 4 bits per pixel.
consteval const lv_font_t* make_font(bytes file, options o) {
  const face f = open(file);
  const float s = static_cast<float>(o.px) / static_cast<float>(f.units);
  std::vector<std::uint8_t> bitmap;
  std::vector<lv_font_fmt_txt_glyph_dsc_t> glyphs(1);  // glyph 0 means "none"

  for (std::uint32_t c = o.first; c <= o.last; ++c) {
    const auto g = glyph_of(f, c);
    lv_font_fmt_txt_glyph_dsc_t dsc{};
    dsc.bitmap_index = static_cast<std::uint32_t>(bitmap.size()) & 0xfffff;
    dsc.adv_w = static_cast<std::uint32_t>(round_of(static_cast<float>(advance_of(f, g)) * s * 16)) & 0xfff;
    if (!is_blank(f, g)) {
      const std::size_t at = glyph_at(f, g);
      const int x0 = floor_of(static_cast<float>(i16(f.d, at + 2)) * s), y0 = floor_of(static_cast<float>(i16(f.d, at + 4)) * s);
      const int x1 = ceil_of(static_cast<float>(i16(f.d, at + 6)) * s), y1 = ceil_of(static_cast<float>(i16(f.d, at + 8)) * s);
      contours parts;
      outline(f, g, matrix{s, 0, 0, -s, static_cast<float>(-x0), static_cast<float>(y1)}, parts);
      raster r{x1 - x0, y1 - y0};
      for (const auto& part : parts) r.contour(part);
      const auto pixels = r.coverage();
      // Packed without a break between rows, the high half of a byte first.
      for (std::size_t i = 0; i < pixels.size(); i += 2)
        bitmap.push_back(static_cast<std::uint8_t>(pixels[i] << 4 | (i + 1 < pixels.size() ? pixels[i + 1] : 0)));
      dsc.box_w = static_cast<std::uint8_t>(x1 - x0);
      dsc.box_h = static_cast<std::uint8_t>(y1 - y0);
      dsc.ofs_x = static_cast<std::int8_t>(x0);
      dsc.ofs_y = static_cast<std::int8_t>(y0);
    }
    glyphs.push_back(dsc);
  }

  lv_font_fmt_txt_cmap_t map{};
  map.range_start = o.first;
  map.range_length = static_cast<std::uint16_t>(o.last - o.first + 1);
  map.glyph_id_start = 1;
  map.type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY;
  const std::vector<lv_font_fmt_txt_cmap_t> maps{map};

  lv_font_fmt_txt_dsc_t dsc{};
  dsc.glyph_bitmap = std::define_static_array(bitmap).data();
  dsc.glyph_dsc = std::define_static_array(glyphs).data();
  dsc.cmaps = std::define_static_array(maps).data();
  dsc.cmap_num = 1;
  dsc.bpp = 4;
  dsc.bitmap_format = LV_FONT_FMT_TXT_PLAIN;
  const std::vector<lv_font_fmt_txt_dsc_t> dscs{dsc};

  const int ascent = ceil_of(static_cast<float>(i16(f.d, f.hhea + 4)) * s);
  const int descent = floor_of(static_cast<float>(i16(f.d, f.hhea + 6)) * s);
  lv_font_t font{};
  font.get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt;
  font.get_glyph_bitmap = lv_font_get_bitmap_fmt_txt;
  font.line_height = ascent - descent;
  font.base_line = -descent;
  font.subpx = LV_FONT_SUBPX_NONE;
  font.static_bitmap = 1;
  font.underline_position = -1;
  font.underline_thickness = 1;
  font.dsc = std::define_static_array(dscs).data();
  const std::vector<lv_font_t> fonts{font};
  return std::define_static_array(fonts).data();
}

}  // namespace spike
