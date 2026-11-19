#pragma once

// A PNG file, decoded in constant evaluation.
//
// It is written for the compiler's interpreter, where every call and every operation
// costs: codes are looked up in tables, buffers have a known size and are written
// through pointers, and each filter has a loop of its own.
//
// Reads 8 bits per channel: grey, grey with alpha, RGB, RGBA and palette images,
// not interlaced. A damaged file that reads outside its data ends the build with
// the compiler's own message about that read.

#include <lv/detail/api.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace lv::detail::png_reader {

using bytes = std::span<const unsigned char>;

consteval std::uint32_t be32(bytes d, std::size_t at) {
  return static_cast<std::uint32_t>(d[at]) << 24 | static_cast<std::uint32_t>(d[at + 1]) << 16 |
         static_cast<std::uint32_t>(d[at + 2]) << 8 | d[at + 3];
}

inline constexpr int fast_bits = 10;

// The codes of one alphabet. `fast` answers for the next ten bits of input: the
// length of the code there and its symbol (length << 9 | symbol), or 0 for a longer
// code, which `counts` and `symbols` find bit by bit.
struct code_table {
  // Plain arrays: the decoder indexes them with int, as the format's arithmetic is written.
  std::uint16_t fast[1 << fast_bits]{};
  std::uint16_t counts[16]{};
  std::uint16_t symbols[288]{};
};

consteval int reversed(int code, int length) {
  int out = 0;
  for (int b = 0; b < length; ++b) out |= ((code >> b) & 1) << (length - 1 - b);
  return out;
}

// Every ten bits that begin with `code` (its bits reversed: input comes low bit first).
consteval void fill_fast(code_table& t, int code, int length, int symbol) {
  for (int at = reversed(code, length); at < (1 << fast_bits); at += 1 << length)
    t.fast[at] = static_cast<std::uint16_t>(length << 9 | symbol);
}

// The table of the canonical code whose symbol i is `lengths[i]` bits long.
consteval code_table table_of(const std::uint8_t* lengths, int n) {
  code_table t;
  for (int i = 0; i < n; ++i) ++t.counts[lengths[i]];
  t.counts[0] = 0;
  std::uint16_t offsets[16]{}, next[16]{};
  for (int i = 1, code = 0; i < 16; ++i) {
    offsets[i] = static_cast<std::uint16_t>(offsets[i - 1] + t.counts[i - 1]);
    code = (code + t.counts[i - 1]) << 1;
    next[i] = static_cast<std::uint16_t>(code);
  }
  for (int i = 0; i < n; ++i) {
    const int length = lengths[i];
    if (length == 0) continue;
    t.symbols[offsets[length]++] = static_cast<std::uint16_t>(i);
    if (length <= fast_bits) fill_fast(t, next[length], length, i);
    ++next[length];
  }
  return t;
}

// The input as bits, low bit first.
struct bit_reader {
  const unsigned char* at;
  const unsigned char* end;
  std::uint64_t hold = 0;
  int count = 0;

  // Makes at least 56 bits ready, while the input lasts.
  consteval void fill() {
    while (count <= 56 && at != end) {
      hold |= static_cast<std::uint64_t>(*at++) << count;
      count += 8;
    }
  }

  consteval std::uint32_t take(int n) {
    const auto v = static_cast<std::uint32_t>(hold & ((1ull << n) - 1));
    hold >>= n;
    count -= n;
    return v;
  }

  // A symbol whose code is longer than the fast table holds.
  consteval int long_symbol(const code_table& t) {
    int code = 0, first = 0, index = 0;
    for (int length = 1; length < 16; ++length) {
      code |= static_cast<int>(take(1));
      const int n = t.counts[length];
      if (code - n < first) return t.symbols[index + (code - first)];
      index += n;
      first = (first + n) << 1;
      code <<= 1;
    }
    return 256;  // no symbol has this code: end the block, and the size check reports the damage
  }

  consteval int symbol(const code_table& t) {
    const std::uint16_t entry = t.fast[hold & ((1u << fast_bits) - 1)];
    if (entry == 0) return long_symbol(t);
    hold >>= entry >> 9;
    count -= entry >> 9;
    return entry & 511;
  }
};

inline constexpr std::uint16_t length_base[] = {3,  4,  5,  6,  7,  8,  9,  10, 11,  13,  15,  17,  19,  23, 27,
                                                31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258};
inline constexpr std::uint8_t length_extra[] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};
inline constexpr std::uint16_t distance_base[] = {1,   2,   3,   4,   5,   7,    9,    13,   17,   25,   33,   49,   65,    97,    129,
                                                  193, 257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577};
inline constexpr std::uint8_t distance_extra[] = {0, 0, 0, 0, 1, 1, 2, 2,  3,  3,  4,  4,  5,  5,  6,
                                                  6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};

// The bytes of one block: literals, and copies of bytes that came before.
consteval unsigned char* block_bytes(bit_reader& in, unsigned char* out, const code_table& literals,
                                     const code_table& distances) {
  for (;;) {
    in.fill();
    const int s = in.symbol(literals);
    if (s == 256) return out;
    if (s < 256) {
      *out++ = static_cast<unsigned char>(s);
      continue;
    }
    const int length = length_base[s - 257] + static_cast<int>(in.take(length_extra[s - 257]));
    const int d = in.symbol(distances);
    const unsigned char* from = out - (distance_base[d] + in.take(distance_extra[d]));
    for (int i = 0; i < length; ++i) *out++ = *from++;
  }
}

// How often a repeat symbol (16, 17 or 18) repeats.
consteval int repeats(bit_reader& in, int symbol) {
  if (symbol == 16) return 3 + static_cast<int>(in.take(2));
  return symbol == 17 ? 3 + static_cast<int>(in.take(3)) : 11 + static_cast<int>(in.take(7));
}

// The code lengths of a block's two alphabets, which are themselves coded.
consteval std::array<std::uint8_t, 320> code_lengths(bit_reader& in, int count, const code_table& code) {
  std::array<std::uint8_t, 320> lengths{};
  for (int i = 0; i < count;) {
    in.fill();
    const int s = in.symbol(code);
    if (s < 16) {
      lengths[static_cast<std::size_t>(i++)] = static_cast<std::uint8_t>(s);
      continue;
    }
    const std::uint8_t value = s == 16 ? lengths[static_cast<std::size_t>(i - 1)] : 0;
    for (int repeat = repeats(in, s); repeat > 0; --repeat) lengths[static_cast<std::size_t>(i++)] = value;
  }
  return lengths;
}

// A block that brings its own codes.
consteval unsigned char* dynamic_block(bit_reader& in, unsigned char* out) {
  constexpr std::uint8_t order[] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
  in.fill();
  const int literals = static_cast<int>(in.take(5)) + 257, distances = static_cast<int>(in.take(5)) + 1;
  const int coded = static_cast<int>(in.take(4)) + 4;
  std::array<std::uint8_t, 19> first{};
  for (int i = 0; i < coded; ++i) {
    in.fill();
    first[order[i]] = static_cast<std::uint8_t>(in.take(3));
  }
  const auto lengths = code_lengths(in, literals + distances, table_of(first.data(), 19));
  return block_bytes(in, out, table_of(lengths.data(), literals), table_of(lengths.data() + literals, distances));
}

// A block with the codes the format fixes.
consteval unsigned char* fixed_block(bit_reader& in, unsigned char* out) {
  std::array<std::uint8_t, 288> literals{};
  for (std::size_t i = 0; i < 288; ++i) literals[i] = i < 144 ? 8 : i < 256 ? 9 : i < 280 ? 7 : 8;
  std::array<std::uint8_t, 30> distances{};
  distances.fill(5);
  return block_bytes(in, out, table_of(literals.data(), 288), table_of(distances.data(), 30));
}

// A block of bytes that are not compressed.
consteval unsigned char* stored_block(bit_reader& in, unsigned char* out) {
  in.take(in.count & 7);  // to the next whole byte
  in.fill();
  std::size_t n = in.take(16);
  in.take(16);
  for (; n > 0; --n) {
    in.fill();
    *out++ = static_cast<unsigned char>(in.take(8));
  }
  return out;
}

// The `size` bytes that the zlib stream `z` holds.
consteval std::vector<unsigned char> inflate(const std::vector<unsigned char>& z, std::size_t size) {
  std::vector<unsigned char> buffer(size);
  unsigned char* out = buffer.data();
  bit_reader in{z.data() + 2, z.data() + z.size()};  // past the two bytes of the zlib header
  for (bool last = false; !last;) {
    in.fill();
    last = in.take(1) != 0;
    const auto type = in.take(2);
    out = type == 0 ? stored_block(in, out) : type == 1 ? fixed_block(in, out) : dynamic_block(in, out);
  }
  if (out != buffer.data() + size) fail("lv: this PNG file is damaged: it does not hold as many pixels as it says.");
  return buffer;
}

// ---- filters: each row says how its bytes were changed before compression -------------

consteval void add_left(unsigned char* row, std::size_t stride, std::size_t step) {
  for (std::size_t x = step; x < stride; ++x) row[x] = static_cast<unsigned char>(row[x] + row[x - step]);
}

consteval void add_up(unsigned char* row, const unsigned char* up, std::size_t stride) {
  for (std::size_t x = 0; x < stride; ++x) row[x] = static_cast<unsigned char>(row[x] + up[x]);
}

consteval void add_average(unsigned char* row, const unsigned char* up, std::size_t stride, std::size_t step) {
  for (std::size_t x = 0; x < step; ++x) row[x] = static_cast<unsigned char>(row[x] + up[x] / 2);
  for (std::size_t x = step; x < stride; ++x) row[x] = static_cast<unsigned char>(row[x] + (row[x - step] + up[x]) / 2);
}

// The one of left, up and upper left that is nearest to left + up - upper left.
consteval int paeth(int a, int b, int c) {
  const int p = a + b - c, pa = p > a ? p - a : a - p, pb = p > b ? p - b : b - p, pc = p > c ? p - c : c - p;
  if (pa <= pb && pa <= pc) return a;
  return pb <= pc ? b : c;
}

consteval void add_paeth(unsigned char* row, const unsigned char* up, std::size_t stride, std::size_t step) {
  for (std::size_t x = 0; x < step; ++x) row[x] = static_cast<unsigned char>(row[x] + up[x]);
  for (std::size_t x = step; x < stride; ++x)
    row[x] = static_cast<unsigned char>(row[x] + paeth(row[x - step], up[x], up[x - step]));
}

// Undoes the filter of one row, in place. `up` is the row above; above the first row are zeros.
consteval void unfilter(unsigned char* row, const unsigned char* up, std::size_t stride, std::size_t step, int filter) {
  if (filter == 1) add_left(row, stride, step);
  if (filter == 2) add_up(row, up, stride);
  if (filter == 3) add_average(row, up, stride, step);
  if (filter == 4) add_paeth(row, up, stride, step);
  if (filter > 4) fail("lv: this PNG file is damaged: a row has a filter that the format does not know.");
}

// ---- the file ---------------------------------------------------------------------------

struct picture {
  std::uint32_t w = 0, h = 0;
  int colour = 0;                        // 0 grey, 2 RGB, 3 palette, 4 grey and alpha, 6 RGBA
  std::size_t step = 0;                  // bytes for each pixel in `rows`
  std::vector<unsigned char> rows;       // for each row: its filter byte, then its pixels
  const unsigned char* shared = nullptr;  // rows that someone else holds: they are not copied (see picture_from)

  // The rows: for each its filter byte, then its pixels.
  consteval const unsigned char* bits() const { return shared != nullptr ? shared : rows.data(); }
  std::vector<unsigned char> palette;    // three bytes for each colour
  // The tRNS chunk. For a palette: the alpha of each colour. For grey and RGB: the one
  // colour that is see-through, each sample in two bytes.
  std::vector<unsigned char> transparency;
};

consteval void check_header(bytes d) {
  constexpr unsigned char signature[] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1a, '\n'};
  for (std::size_t i = 0; i < 8; ++i)
    if (d.size() < 33 || d[i] != signature[i])
      fail("lv: this is not a PNG file. Embed one: `constexpr unsigned char logo_png[] = { #embed \"logo.png\" };`");
  if (d[24] != 8 || d[28] != 0)
    fail("lv: this PNG file has 16 bits or fewer than 8 bits for each channel, or is interlaced. Save it with 8 bits "
         "for each channel and without interlacing.");
}

consteval std::vector<unsigned char> chunk(bytes d, std::size_t body, std::size_t n) {
  return {d.begin() + static_cast<std::ptrdiff_t>(body), d.begin() + static_cast<std::ptrdiff_t>(body + n)};
}

// The size of the picture, read from the first chunk alone.
consteval std::uint32_t width_of(bytes d) { return check_header(d), be32(d, 16); }
consteval std::uint32_t height_of(bytes d) { return check_header(d), be32(d, 20); }

// The bytes of a pixel for each colour type: 0 grey, 2 RGB, 3 palette, 4 grey and alpha, 6 RGBA.
inline constexpr std::size_t pixel_steps[] = {1, 0, 3, 1, 2, 0, 4};

// What the header says, which needs no decoding: the size of the rows once the filters are undone.
consteval std::size_t rows_size(bytes d) {
  return (static_cast<std::size_t>(width_of(d)) * pixel_steps[d[25]] + 1) * height_of(d);
}

// The header, the palette and the see-through colours of a file, and the zlib data if `z` is given.
// The rows are left empty.
consteval picture read_picture(bytes d, std::vector<unsigned char>* z) {
  check_header(d);
  picture out{.w = be32(d, 16), .h = be32(d, 20), .colour = d[25]};
  for (std::size_t at = 8; at + 12 <= d.size();) {
    const std::size_t n = be32(d, at), body = at + 8;
    const auto tag = be32(d, at + 4);
    if (tag == 0x504c5445) out.palette = chunk(d, body, n);        // PLTE
    if (tag == 0x74524e53) out.transparency = chunk(d, body, n);  // tRNS
    if (tag == 0x49444154 && z != nullptr)                        // IDAT
      z->insert(z->end(), d.begin() + static_cast<std::ptrdiff_t>(body), d.begin() + static_cast<std::ptrdiff_t>(body + n));
    at = body + n + 4;
  }
  out.step = pixel_steps[out.colour];
  return out;
}

// The picture with its filters undone.
consteval picture decode(bytes d) {
  std::vector<unsigned char> z;
  picture out = read_picture(d, &z);
  const std::size_t stride = out.w * out.step;
  out.rows = inflate(z, (stride + 1) * out.h);
  const std::vector<unsigned char> zeros(stride);
  for (std::size_t y = 0; y < out.h; ++y) {
    unsigned char* row = out.rows.data() + y * (stride + 1) + 1;
    unfilter(row, y > 0 ? row - stride - 1 : zeros.data(), stride, out.step, row[-1]);
  }
  return out;
}

// The same picture, from rows that were decoded already: rows_size(d) bytes at `rows`, which are read where they are.
consteval picture picture_from(bytes d, const unsigned char* rows) {
  picture out = read_picture(d, nullptr);
  out.shared = rows;
  return out;
}

struct rgba {
  unsigned char r, g, b, a;
};

// The alpha of a grey or RGB pixel: 0 when it has the file's one see-through colour.
// That colour has two bytes for each of its `samples`; with 8 bits the second one counts.
consteval unsigned char keyed_alpha(const picture& p, const unsigned char* at, std::size_t samples) {
  if (p.transparency.size() != 2 * samples) return 255;
  for (std::size_t i = 0; i < samples; ++i)
    if (at[i] != p.transparency[2 * i + 1]) return 255;
  return 0;
}

// The colour of the pixel that starts at `p`.
consteval rgba pixel(const picture& p, const unsigned char* at) {
  if (p.colour == 6) return {at[0], at[1], at[2], at[3]};
  if (p.colour == 2) return {at[0], at[1], at[2], keyed_alpha(p, at, 3)};
  if (p.colour == 4) return {at[0], at[0], at[0], at[1]};
  if (p.colour == 0) return {at[0], at[0], at[0], keyed_alpha(p, at, 1)};
  const std::size_t i = at[0];
  return {p.palette[3 * i], p.palette[3 * i + 1], p.palette[3 * i + 2],
          i < p.transparency.size() ? p.transparency[i] : static_cast<unsigned char>(255)};
}

}  // namespace lv::detail::png_reader
