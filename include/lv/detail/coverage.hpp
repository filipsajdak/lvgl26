#pragma once

// Anti-aliased coverage of an outline, for constant evaluation.
//
// The method is the one of font-rs (https://github.com/raphlinus/font-rs): every edge
// adds its signed area to the cells it crosses, and a running sum along each row
// gives the coverage of each pixel. It needs no sorting and no oversampling.
//
// This file is derived from font-rs, Copyright 2015 Google Inc., and is licensed
// under the Apache License, Version 2.0 (http://www.apache.org/licenses/LICENSE-2.0).
// The rest of lvgl26 is under the MIT licence. See NOTICE.md.

#include <algorithm>
#include <cstddef>
#include <utility>
#include <vector>

namespace lv::detail {

struct outline_point {
  float x, y;
  bool on;  // on the curve; otherwise the control point of a quadratic curve
};

consteval int floor_of(float v) {
  const int i = static_cast<int>(v);
  return static_cast<float>(i) > v ? i - 1 : i;
}
consteval int ceil_of(float v) { return -floor_of(-v); }
consteval int round_of(float v) { return floor_of(v + 0.5f); }

class coverage {
 public:
  consteval coverage(int width, int height)
      : w_(width), h_(height), cells_(static_cast<std::size_t>((width + 2) * height + 2)) {}

  consteval void line(outline_point p0, outline_point p1) {
    if (p0.y == p1.y) return;
    const float dir = p0.y > p1.y ? -1.0f : 1.0f;
    if (p0.y > p1.y) std::swap(p0, p1);
    const float dxdy = (p1.x - p0.x) / (p1.y - p0.y);
    float x = p0.x - std::min(p0.y, 0.0f) * dxdy;  // where the edge enters the first row
    for (int y = std::max(floor_of(p0.y), 0); y < std::min(ceil_of(p1.y), h_); ++y) {
      const float top = std::max(static_cast<float>(y), p0.y), bottom = std::min(static_cast<float>(y + 1), p1.y);
      const float xnext = x + dxdy * (bottom - top);
      row(y, x, xnext, (bottom - top) * dir);
      x = xnext;
    }
  }

  consteval void curve(outline_point p0, outline_point p1, outline_point p2) {
    const float devx = p0.x - 2 * p1.x + p2.x, devy = p0.y - 2 * p1.y + p2.y;
    int n = 1;  // the pieces a curve of this bend needs
    while (static_cast<float>(n * n * n * n) < 3 * (devx * devx + devy * devy)) ++n;
    outline_point p = p0;
    for (int i = 1; i < n; ++i) {
      const float t = static_cast<float>(i) / static_cast<float>(n), u = 1 - t;
      const outline_point next{u * u * p0.x + 2 * u * t * p1.x + t * t * p2.x,
                               u * u * p0.y + 2 * u * t * p1.y + t * t * p2.y, true};
      line(p, next);
      p = next;
    }
    line(p, p2);
  }

  // The coverage of each pixel, row by row, from 0 to `most`: width times height values,
  // written to `out`.
  consteval void pixels(int most, unsigned char* out) const {
    const float* row = cells_.data();
    for (int y = 0; y < h_; ++y, row += w_ + 2) {
      float sum = 0;
      for (int x = 0; x < w_; ++x) {
        sum += row[x];
        const float v = sum < 0 ? -sum : sum;
        *out++ = static_cast<unsigned char>((v > 1 ? 1 : v) * static_cast<float>(most) + 0.5f);
      }
    }
  }

 private:
  consteval float& cell(int x, int y) {
    x = x < 0 ? 0 : x > w_ ? w_ : x;
    return cells_[static_cast<std::size_t>(y * (w_ + 2) + x)];
  }

  // The part of an edge inside row y: from x to xnext, covering `d` of the row's height.
  consteval void row(int y, float x, float xnext, float d) {
    const float x0 = x < xnext ? x : xnext, x1 = x < xnext ? xnext : x;
    const int x0i = floor_of(x0), x1i = ceil_of(x1);
    const float x0floor = static_cast<float>(x0i);
    if (x1i <= x0i + 1) {
      const float xmf = 0.5f * (x + xnext) - x0floor;
      cell(x0i, y) += d - d * xmf;
      cell(x0i + 1, y) += d * xmf;
      return;
    }
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
      cell(x1i - 1, y) += d * (1 - (a1 + static_cast<float>(x1i - x0i - 3) * s) - am);
    }
    cell(x1i, y) += d * am;
  }

  int w_, h_;
  std::vector<float> cells_;
};

}  // namespace lv::detail
